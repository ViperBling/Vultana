#include "RenderGraph.hpp"
#include "Core/VultanaEngine.hpp"

namespace RG
{
    FRenderGraph::FRenderGraph::FRenderGraph(Renderer::FRendererBase *pRenderer)
        : m_ResourceAllocator(pRenderer->GetDevice())
    {
        RHI::FRHIDevice *pDevice = pRenderer->GetDevice();
        m_pGraphicsQueueFence.reset(pDevice->CreateFence("RenderGraph::GraphicsQueueFence"));
        m_pComputeQueueFence.reset(pDevice->CreateFence("RenderGraph::ComputeQueueFence"));
    }

    void FRenderGraph::EndEvent()
    {
        if (!m_EventNames.empty())
        {
            m_EventNames.pop_back();
        }
        else
        {
            m_Passes.back()->EndEvent();
        }
    }

    void FRenderGraph::Clear()
    {
        for (size_t i = 0; i < m_ObjFinalizers.size(); i++)
        {
            m_ObjFinalizers[i].Finalizer(m_ObjFinalizers[i].Object);
        }
        m_ObjFinalizers.clear();

        m_Graph.Clear();

        m_Passes.clear();
        m_ResourceNodes.clear();
        m_Resources.clear();

        m_Allocator.Reset();
        m_ResourceAllocator.Reset();

        m_OutputResources.clear();
    }

    void FRenderGraph::Compile()
    {
        m_Graph.Cull();

        FRenderGraphAsyncResolveContext context;

        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            FRenderGraphPassBase* pass = m_Passes[i];
            if (!pass->IsCulled())
            {
                pass->ResolveAsyncComputeBarrier(m_Graph, context);
            }
        }

        eastl::vector<FDAGEdge*> edges;
        for (size_t i = 0; i < m_ResourceNodes.size(); i++)
        {
            FRenderGraphResourceNode* node = m_ResourceNodes[i];
            if (node->IsCulled())
            {
                continue;
            }

            FRenderGraphResource* resource = node->GetResource();

            m_Graph.GetOutgoingEdges(node, edges);
            for (size_t j = 0; j < edges.size(); j++)
            {
                FRenderGraphEdge* edge = static_cast<FRenderGraphEdge*>(edges[j]);
                FRenderGraphPassBase* pass = static_cast<FRenderGraphPassBase*>(m_Graph.GetNode(edge->GetToNode()));
                if (!pass->IsCulled())
                {
                    resource->Resolve(edge, pass);
                }
            }

            m_Graph.GetIncomingEdges(node, edges);
            for (size_t j = 0; j < edges.size(); j++)
            {
                FRenderGraphEdge* edge = static_cast<FRenderGraphEdge*>(edges[j]);
                FRenderGraphPassBase* pass = static_cast<FRenderGraphPassBase*>(m_Graph.GetNode(edge->GetFromNode()));
                if (!pass->IsCulled())
                {
                    resource->Resolve(edge, pass);
                }
            }
        }

        for (size_t i = 0; i < m_Resources.size(); i++)
        {
            FRenderGraphResource* resource = m_Resources[i];
            if (resource->IsUsed())
            {
                resource->Realize();
            }
        }

        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            FRenderGraphPassBase* pass = m_Passes[i];
            if (!pass->IsCulled())
            {
                pass->ResolveBarriers(m_Graph);
            }
        }
    }

    void FRenderGraph::Execute(Renderer::FRendererBase *pRenderer, RHI::FRHICommandList *pGraphicsCmdList, RHI::FRHICommandList *pComputeCmdList)
    {
        GPU_EVENT_DEBUG(pGraphicsCmdList, "RenderGraph::Execute");

        FRenderGraphPassExecuteContext context = {};
        context.pRenderer = pRenderer;
        context.GraphicsCmdList = pGraphicsCmdList;
        context.ComputeCmdList = pComputeCmdList;
        context.GraphicsFence = m_pGraphicsQueueFence.get();
        context.ComputeFence = m_pComputeQueueFence.get();
        context.InitialGraphicsFenceValue = m_GraphicsQueueFenceValue;
        context.InitialComputeFenceValue = m_ComputeQueueFenceValue;

        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            FRenderGraphPassBase* pass = m_Passes[i];
            pass->Execute(*this, context);
        }
        m_GraphicsQueueFenceValue = context.LastSignalGraphicsFenceValue;
        m_ComputeQueueFenceValue = context.LastSignalComputeFenceValue;

        for (size_t i = 0; i < m_OutputResources.size(); i++)
        {
            const FPresentTarget& target = m_OutputResources[i];
            if (target.Resource->GetFinalState() != target.State)
            {
                target.Resource->Barrier(pGraphicsCmdList, 0, target.Resource->GetFinalState(), target.State);
                target.Resource->SetFinalState(target.State);
            }
        }
        m_OutputResources.clear();
    }

    void FRenderGraph::Present(const FRGHandle &handle, RHI::ERHIAccessFlags finalState)
    {
        assert(handle.IsValid());

        FRenderGraphResource* resource = GetTexture(handle);
        resource->SetExported(true);

        FRenderGraphResourceNode* node = m_ResourceNodes[handle.Node];
        node->MakeTarget();

        FPresentTarget target = {};
        target.Resource = resource;
        target.State = finalState;
        m_OutputResources.push_back(target);
    }

    FRGHandle FRenderGraph::Import(RHI::FRHITexture *texture, RHI::ERHIAccessFlags state)
    {
        auto resource = Allocate<FRGTexture>(m_ResourceAllocator, texture, state);
        auto node = AllocatePOD<FRenderGraphResourceNode>(m_Graph, resource, 0);

        FRGHandle handle;
        handle.Index = (uint16_t)m_Resources.size();
        handle.Node = (uint16_t)m_ResourceNodes.size();
        
        m_Resources.push_back(resource);
        m_ResourceNodes.push_back(node);

        return handle;
    }

    FRGHandle FRenderGraph::Import(RHI::FRHIBuffer *buffer, RHI::ERHIAccessFlags state)
    {
        auto resource = Allocate<FRGBuffer>(m_ResourceAllocator, buffer, state);
        auto node = AllocatePOD<FRenderGraphResourceNode>(m_Graph, resource, 0);

        FRGHandle handle;
        handle.Index = (uint16_t)m_Resources.size();
        handle.Node = (uint16_t)m_ResourceNodes.size();

        m_Resources.push_back(resource);
        m_ResourceNodes.push_back(node);

        return handle;
    }

    FRGTexture *FRenderGraph::GetTexture(const FRGHandle &handle)
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }
        FRenderGraphResource* resource = m_Resources[handle.Index];
        assert(dynamic_cast<FRGTexture*>(resource) != nullptr);
        return static_cast<FRGTexture*>(resource);
    }

    FRGBuffer *FRenderGraph::GetBuffer(const FRGHandle &handle)
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }
        FRenderGraphResource* resource = m_Resources[handle.Index];
        assert(dynamic_cast<FRGBuffer*>(resource) != nullptr);
        return static_cast<FRGBuffer*>(resource);
    }

    eastl::string FRenderGraph::Export()
    {
        return m_Graph.ExportGraphViz();
    }

    FRGHandle FRenderGraph::Read(FRenderGraphPassBase *pass, const FRGHandle &input, RHI::ERHIAccessFlags usage, uint32_t subresource)
    {
        assert(input.IsValid());
        FRenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<FRenderGraphEdge>(m_Graph, inputNode, pass, usage, subresource);

        return input;
    }

    FRGHandle FRenderGraph::Write(FRenderGraphPassBase *pass, const FRGHandle &input, RHI::ERHIAccessFlags usage, uint32_t subresource)
    {
        assert(input.IsValid());
        FRenderGraphResource* resource = m_Resources[input.Index];

        FRenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<FRenderGraphEdge>(m_Graph, inputNode, pass, usage, subresource);

        FRenderGraphResourceNode* outputNode = AllocatePOD<FRenderGraphResourceNode>(m_Graph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<FRenderGraphEdge>(m_Graph, pass, outputNode, usage, subresource);

        FRGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)m_ResourceNodes.size();

        m_ResourceNodes.push_back(outputNode);

        return output;
    }

    FRGHandle FRenderGraph::WriteColor(FRenderGraphPassBase *pass, uint32_t colorIndex, const FRGHandle &input, uint32_t subresource, RHI::ERHIRenderPassLoadOp loadOp, const float4 &clearColor)
    {
        assert(input.IsValid());
        FRenderGraphResource* resource = m_Resources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessRTV;

        FRenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<FRGEdgeColorAttachment>(m_Graph, inputNode, pass, usage, subresource, colorIndex, loadOp, clearColor);

        FRenderGraphResourceNode* outputNode = AllocatePOD<FRenderGraphResourceNode>(m_Graph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<FRGEdgeColorAttachment>(m_Graph, pass, outputNode, usage, subresource, colorIndex, loadOp, clearColor);

        FRGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)m_ResourceNodes.size();

        m_ResourceNodes.push_back(outputNode);

        return output;
    }

    FRGHandle FRenderGraph::WriteDepth(FRenderGraphPassBase *pass, const FRGHandle &input, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil)
    {
        assert(input.IsValid());
        FRenderGraphResource* resource = m_Resources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessDSV;

        FRenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<FRGEdgeDepthAttachment>(m_Graph, inputNode, pass, usage, subresource, depthLoadOp, stencilLoadOp, clearDepth, clearStencil);

        FRenderGraphResourceNode* outputNode = AllocatePOD<FRenderGraphResourceNode>(m_Graph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<FRGEdgeDepthAttachment>(m_Graph, pass, outputNode, usage, subresource, depthLoadOp, stencilLoadOp, clearDepth, clearStencil);

        FRGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)m_ResourceNodes.size();

        m_ResourceNodes.push_back(outputNode);

        return output;
    }

    FRGHandle FRenderGraph::ReadDepth(FRenderGraphPassBase *pass, const FRGHandle &input, uint32_t subresource)
    {
        assert(input.IsValid());
        FRenderGraphResource* resource = m_Resources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessDSVReadOnly;

        FRenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<FRGEdgeDepthAttachment>(m_Graph, inputNode, pass, usage, subresource, RHI::ERHIRenderPassLoadOp::Load, RHI::ERHIRenderPassLoadOp::Load, 0.0f, 0);

        FRenderGraphResourceNode* outputNode = AllocatePOD<FRenderGraphResourceNode>(m_Graph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<FRGEdgeDepthAttachment>(m_Graph, pass, outputNode, usage, subresource, RHI::ERHIRenderPassLoadOp::Load, RHI::ERHIRenderPassLoadOp::Load, 0.0f, 0);

        FRGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)m_ResourceNodes.size();

        m_ResourceNodes.push_back(outputNode);

        return output;
    }
}
