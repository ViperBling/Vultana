#include "RendererBase.hpp"
#include "ForwardPath/ForwardBasePass.hpp"
#include "Core/VultanaEngine.hpp"

namespace Renderer
{
    void RendererBase::BuildRenderGraph(RG::RGHandle &outputColor, RG::RGHandle &outputDepth)
    {
        mpRenderGraph->Clear();

        mpForwardBasePass->Render(mpRenderGraph.get());

        RG::RGHandle sceneColorRT = mpForwardBasePass->GetBasePassColorRT();
        RG::RGHandle sceneDepthRT = mpForwardBasePass->GetBasePassDepthRT();

        OutlinePass(sceneColorRT, sceneDepthRT);
        ObjectIDPass(sceneDepthRT);

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
}