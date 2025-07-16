#include "RendererBase.hpp"
#include "ForwardPath/ForwardBasePass.hpp"
#include "Core/VultanaEngine.hpp"

namespace Renderer
{
    void RendererBase::BuildRenderGraph(RG::RGHandle &outputColor, RG::RGHandle &outputDepth)
    {
        m_pRenderGraph->Clear();

        ImportPrevFrameTextures();

        m_pForwardBasePass->Render(m_pRenderGraph.get());

        RG::RGHandle sceneColorRT = m_pForwardBasePass->GetBasePassColorRT();
        RG::RGHandle sceneDepthRT = m_pForwardBasePass->GetBasePassDepthRT();

        OutlinePass(sceneColorRT, sceneDepthRT);
        ObjectIDPass(sceneDepthRT);
        CopyHistoryPass(sceneDepthRT, /* sceneColorRT, */ sceneColorRT);

        outputColor = sceneColorRT;
        outputDepth = sceneDepthRT;

        m_pRenderGraph->Present(outputColor, RHI::RHIAccessPixelShaderSRV);
        m_pRenderGraph->Present(outputDepth, RHI::RHIAccessDSV);

        m_pRenderGraph->Compile();
    }

    void RendererBase::ObjectIDPass(RG::RGHandle &depth)
    {
        if (!m_bEnableObjectIDRendering) return;
        
        struct FIDPassData
        {
            RG::RGHandle IDTexture;
            RG::RGHandle SceneDepthTexture;
        };

        auto idPass = m_pRenderGraph->AddPass<FIDPassData>("Object ID Pass", RG::RenderPassType::Graphics,
        [&](FIDPassData& data, RG::RGBuilder& builder)
        {
            RG::RGTexture::Desc desc;
            desc.Width = m_RenderWidth;
            desc.Height = m_RenderHeight;
            desc.Format = RHI::ERHIFormat::R32UI;
            
            data.IDTexture = builder.Create<RG::RGTexture>(desc, "ObjectIDTexture");
            data.IDTexture = builder.WriteColor(0, data.IDTexture, 0, RHI::ERHIRenderPassLoadOp::Clear, float4(1000000, 0, 0, 0));
            data.SceneDepthTexture = builder.ReadDepth(depth, 0);
        },
        [&](const FIDPassData& data, RHI::RHICommandList* pCmdList)
        {
            Scene::World* pWorld = Core::VultanaEngine::GetEngineInstance()->GetWorld();
            for (size_t i = 0; i < m_IDPassBatches.size(); i++)
            {
                DrawBatch(pCmdList, m_IDPassBatches[i]);
            }
        });

        depth = idPass->SceneDepthTexture;

        struct FCopyIDPassData
        {
            RG::RGHandle SrcTexture;
        };

        m_pRenderGraph->AddPass<FCopyIDPassData>("Copy ID To Readback Buffer", RG::RenderPassType::Copy,
        [&](FCopyIDPassData& data, RG::RGBuilder& builder)
        {
            data.SrcTexture = builder.Read(idPass->IDTexture);
            builder.SkipCulling();
        },
        [&](const FCopyIDPassData& data, RHI::RHICommandList* pCmdList)
        {
            RG::RGTexture* srcTexture = m_pRenderGraph->GetTexture(data.SrcTexture);

            m_ObjectIDRowPitch = srcTexture->GetTexture()->GetRowPitch();
            uint32_t size = m_ObjectIDRowPitch * srcTexture->GetTexture()->GetDesc().Height;
            if (m_pObjectIDBuffer == nullptr || m_pObjectIDBuffer->GetDesc().Size < size)
            {
                RHI::RHIBufferDesc desc;
                desc.Size = size;
                desc.MemoryType = RHI::ERHIMemoryType::GPUToCPU;
                m_pObjectIDBuffer.reset(m_pDevice->CreateBuffer(desc, "RendererBase::ObjectIDBuffer"));
            }
            pCmdList->CopyTextureToBuffer(srcTexture->GetTexture(), m_pObjectIDBuffer.get(), 0, 0, 0);
        });
    }

    void RendererBase::OutlinePass(RG::RGHandle &color, RG::RGHandle &depth)
    {
        struct FOutlinePassData
        {
            RG::RGHandle OutSceneColorRT;
            RG::RGHandle OutSceneDepthRT;
        };

        auto outlinePass = m_pRenderGraph->AddPass<FOutlinePassData>("Outline Pass", RG::RenderPassType::Graphics,
        [&](FOutlinePassData& data, RG::RGBuilder& builder)
        {
            data.OutSceneColorRT = builder.WriteColor(0, color, 0, RHI::ERHIRenderPassLoadOp::Load);
            data.OutSceneDepthRT = builder.WriteDepth(depth, 0, RHI::ERHIRenderPassLoadOp::Load);
        },
        [&](const FOutlinePassData& data, RHI::RHICommandList* pCmdList)
        {
            for (size_t i = 0; i < m_OutlinePassBatches.size(); i++)
            {
                DrawBatch(pCmdList, m_OutlinePassBatches[i]);
            }
        });

        color = outlinePass->OutSceneColorRT;
        depth = outlinePass->OutSceneDepthRT;
    }

    void RendererBase::CopyHistoryPass(RG::RGHandle sceneDepth, /* RG::RGHandle sceneNormal, */ RG::RGHandle sceneColor)
    {
        struct FCopyDepthPassData
        {
            RG::RGHandle SrcSceneDepthTexture;
            RG::RGHandle DstSceneDepthTexture;
        };

        m_pRenderGraph->AddPass<FCopyDepthPassData>("Copy History Pass", RG::RenderPassType::Compute,
        [&](FCopyDepthPassData& data, RG::RGBuilder& builder)
        {
            data.SrcSceneDepthTexture = builder.Read(sceneDepth);
            data.DstSceneDepthTexture = builder.Write(m_PrevSceneDepthHandle);
        },
        [&](const FCopyDepthPassData& data, RHI::RHICommandList* pCmdList)
        {
            RG::RGTexture* srcSceneDepthTexture = m_pRenderGraph->GetTexture(data.SrcSceneDepthTexture);

            uint32_t cb[2] = { srcSceneDepthTexture->GetSRV()->GetHeapIndex(), srcSceneDepthTexture->GetUAV()->GetHeapIndex() };

            pCmdList->SetPipelineState(m_pCopyDepthPSO);
            pCmdList->SetComputeConstants(0, cb, sizeof(cb));
            pCmdList->Dispatch(DivideRoudingUp(m_RenderWidth, 8), DivideRoudingUp(m_RenderHeight, 8), 1);
        });

        struct FCopyPassData
        {
            // RG::RGHandle SrcSceneNormalTexture;
            // RG::RGHandle DstSceneNormalTexture;

            RG::RGHandle SrcSceneColorTexture;
            RG::RGHandle DstSceneColorTexture;
        };

        m_pRenderGraph->AddPass<FCopyPassData>("Copy History Textures Pass", RG::RenderPassType::Copy,
        [&](FCopyPassData& data, RG::RGBuilder& builder)
        {
            // data.SrcSceneNormalTexture = builder.Read(sceneNormal);
            // data.DstSceneNormalTexture = builder.Write(m_PrevNormalHandle);

            data.SrcSceneColorTexture = builder.Read(sceneColor);
            data.DstSceneColorTexture = builder.Write(m_PrevSceneColorHandle);

            builder.SkipCulling();
        },
        [&](const FCopyPassData& data, RHI::RHICommandList* pCmdList)
        {
            // RG::RGTexture* srcSceneNormalTexture = m_pRenderGraph->GetTexture(data.SrcSceneNormalTexture);
            RG::RGTexture* srcSceneColorTexture = m_pRenderGraph->GetTexture(data.SrcSceneColorTexture);

            // pCmdList->CopyTexture(srcSceneNormalTexture->GetTexture(), m_pPrevNormalTexture->GetTexture(), 0, 0,  0, 0);
            pCmdList->CopyTexture(srcSceneColorTexture->GetTexture(), m_pPrevSceneColorTexture->GetTexture(), 0, 0, 0, 0);
        });
    }

    void RendererBase::ImportPrevFrameTextures()
    {
        if (m_pPrevSceneDepthTexture == nullptr || m_pPrevSceneDepthTexture->GetTexture()->GetDesc().Width != m_RenderWidth || m_pPrevSceneDepthTexture->GetTexture()->GetDesc().Height != m_RenderHeight)
        {
            m_pPrevSceneDepthTexture.reset(CreateTexture2D(m_RenderWidth, m_RenderHeight, 1, RHI::ERHIFormat::R32F, RHI::RHITextureUsageUnorderedAccess, "PrevSceneDepthTexture"));
            // m_pPrevNormalTexture.reset(CreateTexture2D(m_RenderWidth, m_RenderHeight, 1, RHI::ERHIFormat::RGBA8UNORM, RHI::RHITextureUsageUnorderedAccess, "PrevSceneNormalTexture"));
            m_pPrevSceneColorTexture.reset(CreateTexture2D(m_RenderWidth, m_RenderHeight, 1, RHI::ERHIFormat::RGBA16F, RHI::RHITextureUsageUnorderedAccess, "PrevSceneColorTexture"));

            m_bHistoryValid = false;
        }
        else
        {
            m_bHistoryValid = true;
        }

        m_PrevSceneDepthHandle = m_pRenderGraph->Import(m_pPrevSceneDepthTexture->GetTexture(), RHI::RHIAccessComputeUAV);
        // m_PrevNormalHandle = m_pRenderGraph->Import(m_pPrevNormalTexture->GetTexture(), m_bHistoryValid ? RHI::RHIAccessCopyDst : RHI::RHIAccessComputeUAV);
        m_PrevSceneColorHandle = m_pRenderGraph->Import(m_pPrevSceneColorTexture->GetTexture(), m_bHistoryValid ? RHI::RHIAccessCopyDst : RHI::RHIAccessComputeUAV);

        if (!m_bHistoryValid)
        {
            struct FClearHistoryPassData
            {
                RG::RGHandle LinearSceneDepth;
                // RG::RGHandle SceneNormal;
                RG::RGHandle SceneColor;
            };

            auto clearHistoryPass = m_pRenderGraph->AddPass<FClearHistoryPassData>("Clear History Pass", RG::RenderPassType::Compute,
            [&](FClearHistoryPassData& data, RG::RGBuilder& builder)
            {
                data.LinearSceneDepth = builder.Write(m_PrevSceneDepthHandle);
                // data.SceneNormal = builder.Write(m_PrevNormalHandle);
                data.SceneColor = builder.Write(m_PrevSceneColorHandle);
            },
            [=](const FClearHistoryPassData& data, RHI::RHICommandList* pCmdList)
            {
                float clearValue[4] = { 0 };
                pCmdList->ClearUAV(m_pPrevSceneDepthTexture->GetTexture(), m_pPrevSceneDepthTexture->GetUAV(), clearValue);
                // pCmdList->ClearUAV(m_pPrevNormalTexture->GetTexture(), m_pPrevNormalTexture->GetUAV(), clearValue);
                pCmdList->ClearUAV(m_pPrevSceneColorTexture->GetTexture(), m_pPrevSceneColorTexture->GetUAV(), clearValue);
            });
            m_PrevSceneDepthHandle = clearHistoryPass->LinearSceneDepth;
            // m_PrevNormalHandle = clearHistoryPass->SceneNormal;
            m_PrevSceneColorHandle = clearHistoryPass->SceneColor;
        }
    }
}