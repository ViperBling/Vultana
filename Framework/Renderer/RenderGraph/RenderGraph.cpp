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
        if (!mEventNames.empty())
        {
            mEventNames.pop_back();
        }
        else
        {
            mPasses.back()->EndEvent();
        }
    }

    void RenderGraph::Clear()
    {
        for (size_t i = 0; i < mObjFinalizers.size(); i++)
        {
            mObjFinalizers[i].Finalizer(mObjFinalizers[i].Object);
        }
        mObjFinalizers.clear();

        mGraph.Clear();

        mPasses.clear();
        mResourceNodes.clear();
        mResources.clear();

        mAllocator.Reset();
        mResourceAllocator.Reset();

        mOutputResources.clear();
    }

    void RenderGraph::Compile()
    {
        mGraph.Cull();

        RenderGraphAsyncResolveContext context;

        for (size_t i = 0; i < mPasses.size(); i++)
        {
            RenderGraphPassBase* pass = mPasses[i];
            if (!pass->IsCulled())
            {
                pass->ResolveAsyncComputeBarrier(mGraph, context);
            }
        }

        eastl::vector<DAGEdge*> edges;
        for (size_t i = 0; i < mResourceNodes.size(); i++)
        {
            RenderGraphResourceNode* node = mResourceNodes[i];
            if (node->IsCulled())
            {
                continue;
            }

            RenderGraphResource* resource = node->GetResource();

            mGraph.GetOutgoingEdges(node, edges);
            for (size_t j = 0; j < edges.size(); j++)
            {
                RenderGraphEdge* edge = static_cast<RenderGraphEdge*>(edges[j]);
                RenderGraphPassBase* pass = static_cast<RenderGraphPassBase*>(mGraph.GetNode(edge->GetToNode()));
                if (!pass->IsCulled())
                {
                    resource->Resolve(edge, pass);
                }
            }

            mGraph.GetIncomingEdges(node, edges);
            for (size_t j = 0; j < edges.size(); j++)
            {
                RenderGraphEdge* edge = static_cast<RenderGraphEdge*>(edges[j]);
                RenderGraphPassBase* pass = static_cast<RenderGraphPassBase*>(mGraph.GetNode(edge->GetFromNode()));
                if (!pass->IsCulled())
                {
                    resource->Resolve(edge, pass);
                }
            }
        }

        for (size_t i = 0; i < mResources.size(); i++)
        {
            RenderGraphResource* resource = mResources[i];
            if (resource->IsUsed())
            {
                resource->Realize();
            }
        }

        for (size_t i = 0; i < mPasses.size(); i++)
        {
            RenderGraphPassBase* pass = mPasses[i];
            if (!pass->IsCulled())
            {
                pass->ResolveBarriers(mGraph);
            }
        }
    }

    void RenderGraph::Execute(Renderer::RendererBase *pRenderer, RHI::RHICommandList *pGraphicsCmdList, RHI::RHICommandList *pComputeCmdList)
    {
        GPU_EVENT_DEBUG(pGraphicsCmdList, "RenderGraph::Execute");

        RenderGraphPassExecuteContext context = {};
        context.pRenderer = pRenderer;
        context.GraphicsCmdList = pGraphicsCmdList;
        context.ComputeCmdList = pComputeCmdList;
        context.GraphicsFence = mpGraphicsQueueFence.get();
        context.ComputeFence = mpComputeQueueFence.get();
        context.InitialGraphicsFenceValue = mGraphicsQueueFenceValue;
        context.InitialComputeFenceValue = mComputeQueueFenceValue;

        for (size_t i = 0; i < mPasses.size(); i++)
        {
            RenderGraphPassBase* pass = mPasses[i];
            pass->Execute(*this, context);
        }
        mGraphicsQueueFenceValue = context.LastSignalGraphicsFenceValue;
        mComputeQueueFenceValue = context.LastSignalComputeFenceValue;

        for (size_t i = 0; i < mOutputResources.size(); i++)
        {
            const PresentTarget& target = mOutputResources[i];
            if (target.Resource->GetFinalState() != target.State)
            {
                target.Resource->Barrier(pGraphicsCmdList, 0, target.Resource->GetFinalState(), target.State);
                target.Resource->SetFinalState(target.State);
            }
        }
        mOutputResources.clear();
    }

    void RenderGraph::Present(const RGHandle &handle, RHI::ERHIAccessFlags finalState)
    {
        assert(handle.IsValid());

        RenderGraphResource* resource = GetTexture(handle);
        resource->SetExported(true);

        RenderGraphResourceNode* node = mResourceNodes[handle.Node];
        node->MakeTarget();

        PresentTarget target = {};
        target.Resource = resource;
        target.State = finalState;
        mOutputResources.push_back(target);
    }

    RGHandle RenderGraph::Import(RHI::RHITexture *texture, RHI::ERHIAccessFlags state)
    {
        auto resource = Allocate<RGTexture>(mResourceAllocator, texture, state);
        auto node = AllocatePOD<RenderGraphResourceNode>(mGraph, resource, 0);

        RGHandle handle;
        handle.Index = (uint16_t)mResources.size();
        handle.Node = (uint16_t)mResourceNodes.size();
        
        mResources.push_back(resource);
        mResourceNodes.push_back(node);

        return handle;
    }

    RGHandle RenderGraph::Import(RHI::RHIBuffer *buffer, RHI::ERHIAccessFlags state)
    {
        auto resource = Allocate<RGBuffer>(mResourceAllocator, buffer, state);
        auto node = AllocatePOD<RenderGraphResourceNode>(mGraph, resource, 0);

        RGHandle handle;
        handle.Index = (uint16_t)mResources.size();
        handle.Node = (uint16_t)mResourceNodes.size();

        mResources.push_back(resource);
        mResourceNodes.push_back(node);

        return handle;
    }

    RGTexture *RenderGraph::GetTexture(const RGHandle &handle)
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }
        RenderGraphResource* resource = mResources[handle.Index];
        assert(dynamic_cast<RGTexture*>(resource) != nullptr);
        return static_cast<RGTexture*>(resource);
    }

    RGBuffer *RenderGraph::GetBuffer(const RGHandle &handle)
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }
        RenderGraphResource* resource = mResources[handle.Index];
        assert(dynamic_cast<RGBuffer*>(resource) != nullptr);
        return static_cast<RGBuffer*>(resource);
    }

    eastl::string RenderGraph::Export()
    {
        return mGraph.ExportGraphViz();
    }

    RGHandle RenderGraph::Read(RenderGraphPassBase *pass, const RGHandle &input, RHI::ERHIAccessFlags usage, uint32_t subresource)
    {
        assert(input.IsValid());
        RenderGraphResourceNode* inputNode = mResourceNodes[input.Node];
        AllocatePOD<RenderGraphEdge>(mGraph, inputNode, pass, usage, subresource);

        return input;
    }

    RGHandle RenderGraph::Write(RenderGraphPassBase *pass, const RGHandle &input, RHI::ERHIAccessFlags usage, uint32_t subresource)
    {
        assert(input.IsValid());
        RenderGraphResource* resource = mResources[input.Index];

        RenderGraphResourceNode* inputNode = mResourceNodes[input.Node];
        AllocatePOD<RenderGraphEdge>(mGraph, inputNode, pass, usage, subresource);

        RenderGraphResourceNode* outputNode = AllocatePOD<RenderGraphResourceNode>(mGraph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<RenderGraphEdge>(mGraph, pass, outputNode, usage, subresource);

        RGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)mResourceNodes.size();

        mResourceNodes.push_back(outputNode);

        return output;
    }

    RGHandle RenderGraph::WriteColor(RenderGraphPassBase *pass, uint32_t colorIndex, const RGHandle &input, uint32_t subresource, RHI::ERHIRenderPassLoadOp loadOp, const float4 &clearColor)
    {
        assert(input.IsValid());
        RenderGraphResource* resource = mResources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessRTV;

        RenderGraphResourceNode* inputNode = mResourceNodes[input.Node];
        AllocatePOD<RGEdgeColorAttachment>(mGraph, inputNode, pass, usage, subresource, colorIndex, loadOp, clearColor);

        RenderGraphResourceNode* outputNode = AllocatePOD<RenderGraphResourceNode>(mGraph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<RGEdgeColorAttachment>(mGraph, pass, outputNode, usage, subresource, colorIndex, loadOp, clearColor);

        RGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)mResourceNodes.size();

        mResourceNodes.push_back(outputNode);

        return output;
    }

    RGHandle RenderGraph::WriteDepth(RenderGraphPassBase *pass, const RGHandle &input, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil)
    {
        assert(input.IsValid());
        RenderGraphResource* resource = mResources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessDSV;

        RenderGraphResourceNode* inputNode = mResourceNodes[input.Node];
        AllocatePOD<RGEdgeDepthAttachment>(mGraph, inputNode, pass, usage, subresource, depthLoadOp, stencilLoadOp, clearDepth, clearStencil);

        RenderGraphResourceNode* outputNode = AllocatePOD<RenderGraphResourceNode>(mGraph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<RGEdgeDepthAttachment>(mGraph, pass, outputNode, usage, subresource, depthLoadOp, stencilLoadOp, clearDepth, clearStencil);

        RGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)mResourceNodes.size();

        mResourceNodes.push_back(outputNode);

        return output;
    }

    RGHandle RenderGraph::ReadDepth(RenderGraphPassBase *pass, const RGHandle &input, uint32_t subresource)
    {
        assert(input.IsValid());
        RenderGraphResource* resource = mResources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessDSVReadOnly;

        RenderGraphResourceNode* inputNode = mResourceNodes[input.Node];
        AllocatePOD<RGEdgeDepthAttachment>(mGraph, inputNode, pass, usage, subresource, RHI::ERHIRenderPassLoadOp::Load, RHI::ERHIRenderPassLoadOp::Load, 0.0f, 0);

        RenderGraphResourceNode* outputNode = AllocatePOD<RenderGraphResourceNode>(mGraph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<RGEdgeDepthAttachment>(mGraph, pass, outputNode, usage, subresource, RHI::ERHIRenderPassLoadOp::Load, RHI::ERHIRenderPassLoadOp::Load, 0.0f, 0);

        RGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)mResourceNodes.size();

        mResourceNodes.push_back(outputNode);

        return output;
    }
}
