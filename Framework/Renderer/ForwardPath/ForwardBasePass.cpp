#include "ForwardBasePass.hpp"
#include "Renderer/RendererBase.hpp"

namespace Renderer
{
    struct BasePassData
    {
        RG::RGHandle OutColorRT;
        RG::RGHandle OutDepthRT;
    };

    ForwardBasePass::ForwardBasePass(RendererBase *pRenderer)
    {
        mpRenderer = pRenderer;
    }

    RenderBatch &ForwardBasePass::AddBatch()
    {
        LinearAllocator* allocator = mpRenderer->GetConstantAllocator();
        return mInstance.emplace_back(*allocator);
    }

    void ForwardBasePass::Render(RG::RenderGraph *pRenderGraph)
    {
        auto forwardBasePass = pRenderGraph->AddPass<BasePassData>("Forward Base Pass", RG::RenderPassType::Graphics, [&](BasePassData& data, RG::RGBuilder& builder)
        {
            RG::RGTexture::Desc desc;
            desc.Width = mpRenderer->GetRenderWidth();
            desc.Height = mpRenderer->GetRenderHeight();
            desc.Format = RHI::ERHIFormat::RGBA8SRGB;

            data.OutColorRT = builder.Create<RG::RGTexture>(desc, "BasePassColorRT");

            desc.Format = RHI::ERHIFormat::D32F;
            data.OutDepthRT = builder.Create<RG::RGTexture>(desc, "BasePassDepthRT");
        
            data.OutColorRT = builder.WriteColor(0, data.OutColorRT, 0, RHI::ERHIRenderPassLoadOp::Clear, float4(0.0f, 0.0f, 0.0f, 0.0f));
            data.OutDepthRT = builder.WriteDepth(data.OutDepthRT, 0, RHI::ERHIRenderPassLoadOp::Clear, RHI::ERHIRenderPassLoadOp::Clear);
        },
        [&](const BasePassData& data, RHI::RHICommandList* pCmdList)
        {
            for (size_t i = 0; i < mInstance.size(); i++)
            {
                DrawBatch(pCmdList, mInstance[i]);
            }
            mInstance.clear();
        });
        mBasePassColorRT = forwardBasePass->OutColorRT;
        mBasePassDepthRT = forwardBasePass->OutDepthRT;
    }
}