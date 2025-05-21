#include "RendererBase.hpp"
#include "ForwardPath/ForwardBasePass.hpp"
#include "Core/VultanaEngine.hpp"

namespace Renderer
{
    void RendererBase::BuildRenderGraph(RG::RGHandle &outputColor, RG::RGHandle &outputDepth)
    {
        mpRenderGraph->Clear();

        // ImportPrevFrameTextures();

        mpForwardBasePass->Render(mpRenderGraph.get());

        RG::RGHandle sceneColorRT = mpForwardBasePass->GetBasePassColorRT();
        RG::RGHandle sceneDepthRT = mpForwardBasePass->GetBasePassDepthRT();

        OutlinePass(sceneColorRT, sceneDepthRT);
        ObjectIDPass(sceneDepthRT);
        // CopyHistoryPass(sceneDepthRT, /* sceneColorRT, */ sceneColorRT);

        outputColor = sceneColorRT;
        outputDepth = sceneDepthRT;

        mpRenderGraph->Present(outputColor, RHI::RHIAccessPixelShaderSRV);
        mpRenderGraph->Present(outputDepth, RHI::RHIAccessDSVReadOnly);

        mpRenderGraph->Compile();
    }

    void RendererBase::ObjectIDPass(RG::RGHandle &depth)
    {
        if (!mbEnableObjectIDRendering) return;
        
        struct FIDPassData
        {
            RG::RGHandle IDTexture;
            RG::RGHandle SceneDepthTexture;
        };

        auto idPass = mpRenderGraph->AddPass<FIDPassData>("Object ID Pass", RG::RenderPassType::Graphics,
        [&](FIDPassData& data, RG::RGBuilder& builder)
        {
            RG::RGTexture::Desc desc;
            desc.Width = mRenderWidth;
            desc.Height = mRenderHeight;
            desc.Format = RHI::ERHIFormat::R32UI;
            
            data.IDTexture = builder.Create<RG::RGTexture>(desc, "ObjectIDTexture");
            data.IDTexture = builder.WriteColor(0, data.IDTexture, 0, RHI::ERHIRenderPassLoadOp::Clear, float4(1000000, 0, 0, 0));
            data.SceneDepthTexture = builder.ReadDepth(depth, 0);
        },
        [&](const FIDPassData& data, RHI::RHICommandList* pCmdList)
        {
            Scene::World* pWorld = Core::VultanaEngine::GetEngineInstance()->GetWorld();
            for (size_t i = 0; i < mIDPassBatches.size(); i++)
            {
                DrawBatch(pCmdList, mIDPassBatches[i]);
            }
        });

        depth = idPass->SceneDepthTexture;

        struct FCopyIDPassData
        {
            RG::RGHandle SrcTexture;
        };

        mpRenderGraph->AddPass<FCopyIDPassData>("Copy ID To Readback Buffer", RG::RenderPassType::Copy,
        [&](FCopyIDPassData& data, RG::RGBuilder& builder)
        {
            data.SrcTexture = builder.Read(idPass->IDTexture);
            builder.SkipCulling();
        },
        [&](const FCopyIDPassData& data, RHI::RHICommandList* pCmdList)
        {
            RG::RGTexture* srcTexture = mpRenderGraph->GetTexture(data.SrcTexture);

            mObjectIDRowPitch = srcTexture->GetTexture()->GetRowPitch();
            uint32_t size = mObjectIDRowPitch * srcTexture->GetTexture()->GetDesc().Height;
            if (mpObjectIDBuffer == nullptr || mpObjectIDBuffer->GetDesc().Size < size)
            {
                RHI::RHIBufferDesc desc;
                desc.Size = size;
                desc.MemoryType = RHI::ERHIMemoryType::GPUToCPU;
                mpObjectIDBuffer.reset(mpDevice->CreateBuffer(desc, "RendererBase::ObjectIDBuffer"));
            }
            pCmdList->CopyTextureToBuffer(srcTexture->GetTexture(), mpObjectIDBuffer.get(), 0, 0, 0);
        });
    }

    void RendererBase::OutlinePass(RG::RGHandle &color, RG::RGHandle &depth)
    {
        struct FOutlinePassData
        {
            RG::RGHandle OutSceneColorRT;
            RG::RGHandle OutSceneDepthRT;
        };

        auto outlinePass = mpRenderGraph->AddPass<FOutlinePassData>("Outline Pass", RG::RenderPassType::Graphics,
        [&](FOutlinePassData& data, RG::RGBuilder& builder)
        {
            data.OutSceneColorRT = builder.WriteColor(0, color, 0, RHI::ERHIRenderPassLoadOp::Load);
            data.OutSceneDepthRT = builder.WriteDepth(depth, 0, RHI::ERHIRenderPassLoadOp::Load);
        },
        [&](const FOutlinePassData& data, RHI::RHICommandList* pCmdList)
        {
            for (size_t i = 0; i < mOutlinePassBatches.size(); i++)
            {
                DrawBatch(pCmdList, mOutlinePassBatches[i]);
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

        mpRenderGraph->AddPass<FCopyDepthPassData>("Copy History Pass", RG::RenderPassType::Compute,
        [&](FCopyDepthPassData& data, RG::RGBuilder& builder)
        {
            data.SrcSceneDepthTexture = builder.Read(sceneDepth);
            data.DstSceneDepthTexture = builder.Write(m_PrevSceneDepthHandle);
        },
        [&](const FCopyDepthPassData& data, RHI::RHICommandList* pCmdList)
        {
            RG::RGTexture* srcSceneDepthTexture = mpRenderGraph->GetTexture(data.SrcSceneDepthTexture);

            uint32_t cb[2] = { srcSceneDepthTexture->GetSRV()->GetHeapIndex(), srcSceneDepthTexture->GetUAV()->GetHeapIndex() };

            pCmdList->SetPipelineState(mpCopyDepthPSO);
            pCmdList->SetComputeConstants(0, cb, sizeof(cb));
            pCmdList->Dispatch(DivideRoudingUp(mRenderWidth, 8), DivideRoudingUp(mRenderHeight, 8), 1);
        });

        struct FCopyPassData
        {
            // RG::RGHandle SrcSceneNormalTexture;
            // RG::RGHandle DstSceneNormalTexture;

            RG::RGHandle SrcSceneColorTexture;
            RG::RGHandle DstSceneColorTexture;
        };

        mpRenderGraph->AddPass<FCopyPassData>("Copy History Textures Pass", RG::RenderPassType::Copy,
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
            // RG::RGTexture* srcSceneNormalTexture = mpRenderGraph->GetTexture(data.SrcSceneNormalTexture);
            RG::RGTexture* srcSceneColorTexture = mpRenderGraph->GetTexture(data.SrcSceneColorTexture);

            // pCmdList->CopyTexture(mpPrevNormalTexture->GetTexture(), srcSceneNormalTexture->GetTexture(), 0, 0,  0, 0);
            pCmdList->CopyTexture(mpPrevSceneColorTexture->GetTexture(), srcSceneColorTexture->GetTexture(), 0, 0, 0, 0);
        });
    }

    void RendererBase::ImportPrevFrameTextures()
    {
        if (mpPrevSceneDepthTexture == nullptr || mpPrevSceneDepthTexture->GetTexture()->GetDesc().Width != mRenderWidth || mpPrevSceneDepthTexture->GetTexture()->GetDesc().Height != mRenderHeight)
        {
            mpPrevSceneDepthTexture.reset(CreateTexture2D(mRenderWidth, mRenderHeight, 1, RHI::ERHIFormat::R32F, RHI::RHITextureUsageUnorderedAccess, "PrevSceneDepthTexture"));
            // mpPrevNormalTexture.reset(CreateTexture2D(mRenderWidth, mRenderHeight, 1, RHI::ERHIFormat::RGBA8UNORM, RHI::RHITextureUsageUnorderedAccess, "PrevSceneNormalTexture"));
            mpPrevSceneColorTexture.reset(CreateTexture2D(mRenderWidth, mRenderHeight, 1, RHI::ERHIFormat::RGBA16F, RHI::RHITextureUsageUnorderedAccess, "PrevSceneColorTexture"));

            mbHistoryValid = false;
        }
        else
        {
            mbHistoryValid = true;
        }

        m_PrevSceneDepthHandle = mpRenderGraph->Import(mpPrevSceneDepthTexture->GetTexture(), RHI::RHIAccessComputeUAV);
        // m_PrevNormalHandle = mpRenderGraph->Import(mpPrevNormalTexture->GetTexture(), mbHistoryValid ? RHI::RHIAccessCopyDst : RHI::RHIAccessComputeUAV);
        m_PrevSceneColorHandle = mpRenderGraph->Import(mpPrevSceneColorTexture->GetTexture(), mbHistoryValid ? RHI::RHIAccessCopyDst : RHI::RHIAccessComputeUAV);

        if (!mbHistoryValid)
        {
            struct FClearHistoryPassData
            {
                RG::RGHandle LinearSceneDepth;
                // RG::RGHandle SceneNormal;
                RG::RGHandle SceneColor;
            };

            auto clearHistoryPass = mpRenderGraph->AddPass<FClearHistoryPassData>("Clear History Pass", RG::RenderPassType::Compute,
            [&](FClearHistoryPassData& data, RG::RGBuilder& builder)
            {
                data.LinearSceneDepth = builder.Write(m_PrevSceneDepthHandle);
                // data.SceneNormal = builder.Write(m_PrevNormalHandle);
                data.SceneColor = builder.Write(m_PrevSceneColorHandle);
            },
            [=](const FClearHistoryPassData& data, RHI::RHICommandList* pCmdList)
            {
                float clearValue[4] = { 0 };
                pCmdList->ClearUAV(mpPrevSceneDepthTexture->GetTexture(), mpPrevSceneDepthTexture->GetUAV(), clearValue);
                // pCmdList->ClearUAV(mpPrevNormalTexture->GetTexture(), mpPrevNormalTexture->GetUAV(), clearValue);
                pCmdList->ClearUAV(mpPrevSceneColorTexture->GetTexture(), mpPrevSceneColorTexture->GetUAV(), clearValue);
            });
            m_PrevSceneDepthHandle = clearHistoryPass->LinearSceneDepth;
            // m_PrevNormalHandle = clearHistoryPass->SceneNormal;
            m_PrevSceneColorHandle = clearHistoryPass->SceneColor;
        }
    }
}