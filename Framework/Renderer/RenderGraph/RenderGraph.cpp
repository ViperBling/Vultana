#include "RenderGraph.hpp"
#include "Core/VultanaEngine.hpp"

namespace RG
{
    RenderGraph::RenderGraph::RenderGraph(Renderer::RendererBase *pRenderer)
        : m_ResourceAllocator(pRenderer->GetDevice())
    {
        RHI::RHIDevice *pDevice = pRenderer->GetDevice();
        m_pGraphicsQueueFence.reset(pDevice->CreateFence("RenderGraph::GraphicsQueueFence"));
        m_pComputeQueueFence.reset(pDevice->CreateFence("RenderGraph::ComputeQueueFence"));
    }

    void RenderGraph::EndEvent()
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

    void RenderGraph::Clear()
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

    void RenderGraph::Compile()
    {
        m_Graph.Cull();

        RenderGraphAsyncResolveContext context;

        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            RenderGraphPassBase* pass = m_Passes[i];
            if (!pass->IsCulled())
            {
                pass->ResolveAsyncComputeBarrier(m_Graph, context);
            }
        }

        eastl::vector<DAGEdge*> edges;
        for (size_t i = 0; i < m_ResourceNodes.size(); i++)
        {
            RenderGraphResourceNode* node = m_ResourceNodes[i];
            if (node->IsCulled())
            {
                continue;
            }

            RenderGraphResource* resource = node->GetResource();

            m_Graph.GetOutgoingEdges(node, edges);
            for (size_t j = 0; j < edges.size(); j++)
            {
                RenderGraphEdge* edge = static_cast<RenderGraphEdge*>(edges[j]);
                RenderGraphPassBase* pass = static_cast<RenderGraphPassBase*>(m_Graph.GetNode(edge->GetToNode()));
                if (!pass->IsCulled())
                {
                    resource->Resolve(edge, pass);
                }
            }

            m_Graph.GetIncomingEdges(node, edges);
            for (size_t j = 0; j < edges.size(); j++)
            {
                RenderGraphEdge* edge = static_cast<RenderGraphEdge*>(edges[j]);
                RenderGraphPassBase* pass = static_cast<RenderGraphPassBase*>(m_Graph.GetNode(edge->GetFromNode()));
                if (!pass->IsCulled())
                {
                    resource->Resolve(edge, pass);
                }
            }
        }

        for (size_t i = 0; i < m_Resources.size(); i++)
        {
            RenderGraphResource* resource = m_Resources[i];
            if (resource->IsUsed())
            {
                resource->Realize();
            }
        }

        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            RenderGraphPassBase* pass = m_Passes[i];
            if (!pass->IsCulled())
            {
                pass->ResolveBarriers(m_Graph);
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
        context.GraphicsFence = m_pGraphicsQueueFence.get();
        context.ComputeFence = m_pComputeQueueFence.get();
        context.InitialGraphicsFenceValue = m_GraphicsQueueFenceValue;
        context.InitialComputeFenceValue = m_ComputeQueueFenceValue;

        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            RenderGraphPassBase* pass = m_Passes[i];
            pass->Execute(*this, context);
        }
        m_GraphicsQueueFenceValue = context.LastSignalGraphicsFenceValue;
        m_ComputeQueueFenceValue = context.LastSignalComputeFenceValue;

        for (size_t i = 0; i < m_OutputResources.size(); i++)
        {
            const PresentTarget& target = m_OutputResources[i];
            if (target.Resource->GetFinalState() != target.State)
            {
                target.Resource->Barrier(pGraphicsCmdList, 0, target.Resource->GetFinalState(), target.State);
                target.Resource->SetFinalState(target.State);
            }
        }
        m_OutputResources.clear();
    }

    void RenderGraph::Present(const RGHandle &handle, RHI::ERHIAccessFlags finalState)
    {
        assert(handle.IsValid());

        RenderGraphResource* resource = GetTexture(handle);
        resource->SetExported(true);

        RenderGraphResourceNode* node = m_ResourceNodes[handle.Node];
        node->MakeTarget();

        PresentTarget target = {};
        target.Resource = resource;
        target.State = finalState;
        m_OutputResources.push_back(target);
    }

    RGHandle RenderGraph::Import(RHI::RHITexture *texture, RHI::ERHIAccessFlags state)
    {
        auto resource = Allocate<RGTexture>(m_ResourceAllocator, texture, state);
        auto node = AllocatePOD<RenderGraphResourceNode>(m_Graph, resource, 0);

        RGHandle handle;
        handle.Index = (uint16_t)m_Resources.size();
        handle.Node = (uint16_t)m_ResourceNodes.size();
        
        m_Resources.push_back(resource);
        m_ResourceNodes.push_back(node);

        return handle;
    }

    RGHandle RenderGraph::Import(RHI::RHIBuffer *buffer, RHI::ERHIAccessFlags state)
    {
        auto resource = Allocate<RGBuffer>(m_ResourceAllocator, buffer, state);
        auto node = AllocatePOD<RenderGraphResourceNode>(m_Graph, resource, 0);

        RGHandle handle;
        handle.Index = (uint16_t)m_Resources.size();
        handle.Node = (uint16_t)m_ResourceNodes.size();

        m_Resources.push_back(resource);
        m_ResourceNodes.push_back(node);

        return handle;
    }

    RGTexture *RenderGraph::GetTexture(const RGHandle &handle)
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }
        RenderGraphResource* resource = m_Resources[handle.Index];
        assert(dynamic_cast<RGTexture*>(resource) != nullptr);
        return static_cast<RGTexture*>(resource);
    }

    RGBuffer *RenderGraph::GetBuffer(const RGHandle &handle)
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }
        RenderGraphResource* resource = m_Resources[handle.Index];
        assert(dynamic_cast<RGBuffer*>(resource) != nullptr);
        return static_cast<RGBuffer*>(resource);
    }

    eastl::string RenderGraph::Export()
    {
        return m_Graph.ExportGraphViz();
    }

    RGHandle RenderGraph::Read(RenderGraphPassBase *pass, const RGHandle &input, RHI::ERHIAccessFlags usage, uint32_t subresource)
    {
        assert(input.IsValid());
        RenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<RenderGraphEdge>(m_Graph, inputNode, pass, usage, subresource);

        return input;
    }

    RGHandle RenderGraph::Write(RenderGraphPassBase *pass, const RGHandle &input, RHI::ERHIAccessFlags usage, uint32_t subresource)
    {
        assert(input.IsValid());
        RenderGraphResource* resource = m_Resources[input.Index];

        RenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<RenderGraphEdge>(m_Graph, inputNode, pass, usage, subresource);

        RenderGraphResourceNode* outputNode = AllocatePOD<RenderGraphResourceNode>(m_Graph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<RenderGraphEdge>(m_Graph, pass, outputNode, usage, subresource);

        RGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)m_ResourceNodes.size();

        m_ResourceNodes.push_back(outputNode);

        return output;
    }

    RGHandle RenderGraph::WriteColor(RenderGraphPassBase *pass, uint32_t colorIndex, const RGHandle &input, uint32_t subresource, RHI::ERHIRenderPassLoadOp loadOp, const float4 &clearColor)
    {
        assert(input.IsValid());
        RenderGraphResource* resource = m_Resources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessRTV;

        RenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<RGEdgeColorAttachment>(m_Graph, inputNode, pass, usage, subresource, colorIndex, loadOp, clearColor);

        RenderGraphResourceNode* outputNode = AllocatePOD<RenderGraphResourceNode>(m_Graph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<RGEdgeColorAttachment>(m_Graph, pass, outputNode, usage, subresource, colorIndex, loadOp, clearColor);

        RGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)m_ResourceNodes.size();

        m_ResourceNodes.push_back(outputNode);

        return output;
    }

    RGHandle RenderGraph::WriteDepth(RenderGraphPassBase *pass, const RGHandle &input, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil)
    {
        assert(input.IsValid());
        RenderGraphResource* resource = m_Resources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessDSV;

        RenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<RGEdgeDepthAttachment>(m_Graph, inputNode, pass, usage, subresource, depthLoadOp, stencilLoadOp, clearDepth, clearStencil);

        RenderGraphResourceNode* outputNode = AllocatePOD<RenderGraphResourceNode>(m_Graph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<RGEdgeDepthAttachment>(m_Graph, pass, outputNode, usage, subresource, depthLoadOp, stencilLoadOp, clearDepth, clearStencil);

        RGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)m_ResourceNodes.size();

        m_ResourceNodes.push_back(outputNode);

        return output;
    }

    RGHandle RenderGraph::ReadDepth(RenderGraphPassBase *pass, const RGHandle &input, uint32_t subresource)
    {
        assert(input.IsValid());
        RenderGraphResource* resource = m_Resources[input.Index];

        RHI::ERHIAccessFlags usage = RHI::RHIAccessDSVReadOnly;

        RenderGraphResourceNode* inputNode = m_ResourceNodes[input.Node];
        AllocatePOD<RGEdgeDepthAttachment>(m_Graph, inputNode, pass, usage, subresource, RHI::ERHIRenderPassLoadOp::Load, RHI::ERHIRenderPassLoadOp::Load, 0.0f, 0);

        RenderGraphResourceNode* outputNode = AllocatePOD<RenderGraphResourceNode>(m_Graph, resource, inputNode->GetVersion() + 1);
        AllocatePOD<RGEdgeDepthAttachment>(m_Graph, pass, outputNode, usage, subresource, RHI::ERHIRenderPassLoadOp::Load, RHI::ERHIRenderPassLoadOp::Load, 0.0f, 0);

        RGHandle output;
        output.Index = input.Index;
        output.Node = (uint16_t)m_ResourceNodes.size();

        m_ResourceNodes.push_back(outputNode);

        return output;
    }
}
