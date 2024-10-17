#include "RenderGraph.hpp"
#include "Core/VultanaEngine.hpp"

namespace RG
{
    RenderGraph::RenderGraph::RenderGraph(Renderer::RendererBase *pRenderer)
        : mResourceAllocator(pRenderer->GetDevice())
    {
        RHI::RHIDevice *pDevice = pRenderer->GetDevice();
        mpGraphicsQueueFence.reset(pDevice->CreateFence("RenderGraph::GraphicsQueueFence"));
        mpComputeQueueFence.reset(pDevice->CreateFence("RenderGraph::ComputeQueueFence"));
    }

    void RenderGraph::EndEvent()
    {
    }

    void RenderGraph::Clear()
    {
    }

    void RenderGraph::Compile()
    {
    }

    void RenderGraph::Execute(Renderer::RendererBase *pRenderer, RHI::RHICommandList *pGraphicsCmdList, RHI::RHICommandList *pComputeCmdList)
    {
    }

    void RenderGraph::Present(const RGHandle &handle, RHI::ERHIAccessFlags finalState)
    {
    }

    RGHandle RenderGraph::Import(RHI::RHITexture *texture, RHI::ERHIAccessFlags state)
    {
        return RGHandle();
    }

    RGHandle RenderGraph::Import(RHI::RHIBuffer *buffer, RHI::ERHIAccessFlags state)
    {
        return RGHandle();
    }

    RGTexture *RenderGraph::GetTexture(const RGHandle &handle)
    {
        return nullptr;
    }

    RGBuffer *RenderGraph::GetBuffer(const RGHandle &handle)
    {
        return nullptr;
    }

    RGHandle RenderGraph::Read(RenderGraphPassBase *pass, const RGHandle &input, RHI::ERHIAccessFlags usage, uint32_t subresource)
    {
        return RGHandle();
    }

    RGHandle RenderGraph::Write(RenderGraphPassBase *pass, const RGHandle &input, RHI::ERHIAccessFlags usage, uint32_t subresource)
    {
        return RGHandle();
    }

    RGHandle RenderGraph::WriteColor(RenderGraphPassBase *pass, uint32_t colorIndex, const RGHandle &input, uint32_t subresource, RHI::ERHIRenderPassLoadOp loadOp, const float4 &clearColor)
    {
        return RGHandle();
    }

    RGHandle RenderGraph::WriteDepth(RenderGraphPassBase *pass, const RGHandle &input, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil)
    {
        return RGHandle();
    }

    RGHandle RenderGraph::ReadDepth(RenderGraphPassBase *pass, const RGHandle &input, uint32_t subresource)
    {
        return RGHandle();
    }
}
