#include "RenderGraphPass.hpp"
#include "RenderGraph.hpp"
#include "Renderer/RendererBase.hpp"

#include <algorithm>

namespace RG
{
    RenderGraphPassBase::RenderGraphPassBase(const eastl::string &name, RenderPassType type, DirectedAcyclicGraph &graph)
        : DAGNode(graph)
    {
        m_Name = name;
        m_Type = type;
    }

    void RenderGraphPassBase::ResolveBarriers(const DirectedAcyclicGraph &graph)
    {
        eastl::vector<DAGEdge*> edges;
        eastl::vector<DAGEdge*> resIncoming;
        eastl::vector<DAGEdge*> resOutgoing;

        graph.GetIncomingEdges(this, edges);
        for (size_t i = 0; i < edges.size(); i++)
        {
            RenderGraphEdge *edge = static_cast<RenderGraphEdge *>(edges[i]);
            assert(edge->GetToNode() == this->GetID());

            RenderGraphResourceNode* resourceNode = static_cast<RenderGraphResourceNode*>(graph.GetNode(edge->GetFromNode()));
            RenderGraphResource* resource = resourceNode->GetResource();

            graph.GetIncomingEdges(resourceNode, resIncoming);
            graph.GetOutgoingEdges(resourceNode, resOutgoing);
            assert(resIncoming.size() <= 1);
            assert(resOutgoing.size() >= 1);

            RHI::ERHIAccessFlags oldState = RHI::RHIAccessPresent;
            RHI::ERHIAccessFlags newState = edge->GetUsage();

            if (resOutgoing.size() > 1)
            {
                for (int i = (int)resOutgoing.size() - 1; i >= 0; --i)
                {
                    uint32_t subresource = ((RenderGraphEdge*)resOutgoing[i])->GetSubresource();
                    DAGNodeID passID = resOutgoing[i]->GetToNode();
                    if (subresource == edge->GetSubresource() && passID < this->GetID() && !graph.GetNode(passID)->IsCulled())
                    {
                        oldState = ((RenderGraphEdge*)resOutgoing[i])->GetUsage();
                        break;
                    }
                }
            }

            if (oldState == RHI::RHIAccessPresent)
            {
                if (resIncoming.empty())
                {
                    assert(resourceNode->GetVersion() == 0);
                    oldState = resource->GetInitialState();
                }
                else
                {
                    oldState = ((RenderGraphEdge*)resIncoming[0])->GetUsage();
                }
            }
            
            bool isAliased = false;
            RHI::ERHIAccessFlags aliasState;

            if (resource->IsOverlapping() && resource->GetFirstPassID() == this->GetID())
            {
                RHI::RHIResource* aliasedRes = resource->GetAliasedPrevResource(aliasState);
                if (aliasedRes)
                {
                    m_AliasDiscardBarriers.push_back({ aliasedRes, aliasState, newState | RHI::RHIAccessDiscard });
                    isAliased = true;
                }
            }
            if (oldState != newState || isAliased)
            {
                ResourceBarrier barrier;
                barrier.Resource = resource;
                barrier.Subresource = edge->GetSubresource();
                barrier.OldState = oldState;
                barrier.NewState = newState;

                if (isAliased)
                {
                    barrier.OldState |= aliasState | RHI::RHIAccessDiscard;
                }
                m_ResourceBarriers.push_back(barrier);
            }
        }

        graph.GetOutgoingEdges(this, edges);
        for (size_t i = 0; i < edges.size(); i++)
        {
            RenderGraphEdge* edge = static_cast<RenderGraphEdge*>(edges[i]);
            assert(edge->GetFromNode() == this->GetID());

            RHI::ERHIAccessFlags newState = edge->GetUsage();

            if (newState == RHI::RHIAccessRTV)
            {
                assert(dynamic_cast<RGEdgeColorAttachment*>(edge) != nullptr);
                RGEdgeColorAttachment* colorRT = static_cast<RGEdgeColorAttachment*>(edge);
                m_pColorRT[colorRT->GetColorIndex()] = colorRT;
            }
            else if (newState == RHI::RHIAccessDSV || newState == RHI::RHIAccessDSVReadOnly)
            {
                assert(dynamic_cast<RGEdgeDepthAttachment*>(edge) != nullptr);
                m_pDepthRT = static_cast<RGEdgeDepthAttachment*>(edge);
            }
        }
    }

    void RenderGraphPassBase::ResolveAsyncComputeBarrier(const DirectedAcyclicGraph &graph, RenderGraphAsyncResolveContext &context)
    {
        if (m_Type == RenderPassType::AsyncCompute)
        {
            eastl::vector<DAGEdge*> edges;
            eastl::vector<DAGEdge*> resIncoming;
            eastl::vector<DAGEdge*> resOutgoing;

            graph.GetIncomingEdges(this, edges);
            for (size_t i = 0; i < edges.size(); i++)
            {
                RenderGraphEdge* edge = static_cast<RenderGraphEdge*>(edges[i]);
                assert(edge->GetToNode() == this->GetID());

                RenderGraphResourceNode* resourceNode = static_cast<RenderGraphResourceNode*>(graph.GetNode(edge->GetFromNode()));

                graph.GetIncomingEdges(resourceNode, resIncoming);
                assert(resIncoming.size() <= 1);

                if (!resIncoming.empty())
                {
                    RenderGraphPassBase* prePass = static_cast<RenderGraphPassBase*>(graph.GetNode(resIncoming[0]->GetFromNode()));
                    if (!prePass->IsCulled() && prePass->GetType() != RenderPassType::AsyncCompute)
                    {
                        context.PreGraphicsQueuePasses.push_back(prePass->GetID());
                    }
                }
            }

            graph.GetOutgoingEdges(this, edges);
            for (size_t i = 0; i < edges.size(); i++)
            {
                RenderGraphEdge* edge = static_cast<RenderGraphEdge*>(edges[i]);
                assert(edge->GetFromNode() == this->GetID());

                RenderGraphResourceNode* resourceNode = static_cast<RenderGraphResourceNode*>(graph.GetNode(edge->GetToNode()));

                graph.GetOutgoingEdges(resourceNode, resOutgoing);

                for (size_t j = 0; j < resOutgoing.size(); j++)
                {
                    RenderGraphPassBase* postPass = static_cast<RenderGraphPassBase*>(graph.GetNode(resOutgoing[j]->GetToNode()));
                    if (!postPass->IsCulled() && postPass->GetType() != RenderPassType::AsyncCompute)
                    {
                        context.PostGraphicsQueuePasses.push_back(postPass->GetID());
                    }
                }
            }
        }
        else
        {
            if (!context.ComputeQueuePasses.empty())
            {
                if (!context.PreGraphicsQueuePasses.empty())
                {
                    DAGNodeID graphicsPassToWaitID = *eastl::max_element(context.PreGraphicsQueuePasses.begin(), context.PreGraphicsQueuePasses.end());

                    RenderGraphPassBase* graphicsPassToWait = static_cast<RenderGraphPassBase*>(graph.GetNode(graphicsPassToWaitID));
                    if (graphicsPassToWait->m_SignalValue == -1)
                    {
                        graphicsPassToWait->m_SignalValue = ++context.GraphicsFence;
                    }

                    RenderGraphPassBase* computePass = static_cast<RenderGraphPassBase*>(graph.GetNode(context.ComputeQueuePasses[0]));
                    computePass->m_WaitValue = graphicsPassToWait->m_SignalValue;

                    for (size_t i = 0; i < context.ComputeQueuePasses.size(); i++)
                    {
                        RenderGraphPassBase* pass = static_cast<RenderGraphPassBase*>(graph.GetNode(context.ComputeQueuePasses[i]));
                        pass->m_WaitGraphicsPass = graphicsPassToWaitID;
                    }
                }

                if (!context.PostGraphicsQueuePasses.empty())
                {
                    DAGNodeID graphicsPassToSignalID = *eastl::min_element(context.PostGraphicsQueuePasses.begin(), context.PostGraphicsQueuePasses.end());

                    RenderGraphPassBase* computePass = static_cast<RenderGraphPassBase*>(graph.GetNode(context.ComputeQueuePasses.back()));
                    if (computePass->m_SignalValue == -1)
                    {
                        computePass->m_SignalValue = ++context.ComputeFence;
                    }

                    RenderGraphPassBase* graphicsPassToSignal = static_cast<RenderGraphPassBase*>(graph.GetNode(graphicsPassToSignalID));
                    graphicsPassToSignal->m_WaitValue = computePass->m_SignalValue;

                    for (size_t i = 0; i < context.ComputeQueuePasses.size(); i++)
                    {
                        RenderGraphPassBase* pass = static_cast<RenderGraphPassBase*>(graph.GetNode(context.ComputeQueuePasses[i]));
                        pass->m_SignalGraphicsPass = graphicsPassToSignalID;
                    }
                }

                context.ComputeQueuePasses.clear();
                context.PreGraphicsQueuePasses.clear();
                context.PostGraphicsQueuePasses.clear();
            }
        }
    }

    void RenderGraphPassBase::Execute(const RenderGraph &graph, RenderGraphPassExecuteContext &context)
    {
        RHI::RHICommandList* pCmdList = m_Type == RenderPassType::AsyncCompute ? context.ComputeCmdList : context.GraphicsCmdList;

        if (m_WaitValue != -1)
        {
            pCmdList->End();
            pCmdList->Submit();
            
            pCmdList->Begin();
            context.pRenderer->SetupGlobalConstants(pCmdList);

            if (m_Type == RenderPassType::AsyncCompute)
            {
                pCmdList->Wait(context.GraphicsFence, context.InitialGraphicsFenceValue + m_WaitValue);
            }
            else
            {
                pCmdList->Wait(context.ComputeFence, context.InitialComputeFenceValue + m_WaitValue);
            }
        }

        for (size_t i = 0; i < m_EventNames.size(); i++)
        {
            context.GraphicsCmdList->BeginEvent(m_EventNames[i]);
            // TODO : Profiler
        }
        if (!IsCulled())
        {
            GPU_EVENT_DEBUG(pCmdList, m_Name);

            Begin(graph, pCmdList);
            ExecuteImpl(pCmdList);
            End(pCmdList);
        }

        for (uint32_t i = 0; i < m_EndEventNum; i++)
        {
            context.GraphicsCmdList->EndEvent();
            // TODO : Profiler
        }

        if (m_SignalValue != -1)
        {
            pCmdList->End();
            if (m_Type == RenderPassType::AsyncCompute)
            {
                pCmdList->Signal(context.ComputeFence, context.InitialComputeFenceValue + m_SignalValue);
                context.LastSignalComputeFenceValue = context.InitialComputeFenceValue + m_SignalValue;
            }
            else
            {
                pCmdList->Signal(context.GraphicsFence, context.InitialGraphicsFenceValue + m_SignalValue);
                context.LastSignalGraphicsFenceValue = context.InitialGraphicsFenceValue + m_SignalValue;
            }
            pCmdList->Submit();

            pCmdList->Begin();
            context.pRenderer->SetupGlobalConstants(pCmdList);
        }
    }

    void RenderGraphPassBase::Begin(const RenderGraph &graph, RHI::RHICommandList *pCmdList)
    {
        for (size_t i = 0; i < m_AliasDiscardBarriers.size(); i++)
        {
            const AliasDiscardBarrier& barrier = m_AliasDiscardBarriers[i];

            if (barrier.Resource->IsTexture())
            {
                pCmdList->TextureBarrier((RHI::RHITexture*)barrier.Resource, RHI::RHI_ALL_SUB_RESOURCE, barrier.AccessBefore, barrier.AccessAfter);
            }
            else
            {
                pCmdList->BufferBarrier((RHI::RHIBuffer*)barrier.Resource, barrier.AccessBefore, barrier.AccessAfter);
            }
        }

        for (size_t i = 0; i < m_ResourceBarriers.size(); i++)
        {
            const ResourceBarrier& barrier = m_ResourceBarriers[i];
            barrier.Resource->Barrier(pCmdList, barrier.Subresource, barrier.OldState, barrier.NewState);
        }

        if (HasRHIRenderPass())
        {
            RHI::RHIRenderPassDesc rpDesc;

            for (int i = 0; i < RHI::RHI_MAX_COLOR_ATTACHMENT_COUNT; i++)
            {
                if (m_pColorRT[i] != nullptr)
                {
                    RenderGraphResourceNode* node = static_cast<RenderGraphResourceNode*>(graph.GetDAG().GetNode(m_pColorRT[i]->GetToNode()));
                    RHI::RHITexture* texture = static_cast<RGTexture*>(node->GetResource())->GetTexture();

                    uint32_t mip, slice;
                    RHI::DecomposeSubresource(texture->GetDesc(), m_pColorRT[i]->GetSubresource(), mip, slice);

                    rpDesc.Color[i].Texture = texture;
                    rpDesc.Color[i].MipSlice = mip;
                    rpDesc.Color[i].ArraySlice = slice;
                    rpDesc.Color[i].LoadOp = m_pColorRT[i]->GetLoadOp();
                    rpDesc.Color[i].StoreOp = node->IsCulled() ? RHI::ERHIRenderPassStoreOp::DontCare : RHI::ERHIRenderPassStoreOp::Store;
                    memcpy(rpDesc.Color[i].ClearColor, m_pColorRT[i]->GetClearColor(), sizeof(float) * 4);
                }
            }

            if (m_pDepthRT != nullptr)
            {
                RenderGraphResourceNode* node = static_cast<RenderGraphResourceNode*>(graph.GetDAG().GetNode(m_pDepthRT->GetToNode()));
                RHI::RHITexture* texture = static_cast<RGTexture*>(node->GetResource())->GetTexture();

                uint32_t mip, slice;
                RHI::DecomposeSubresource(texture->GetDesc(), m_pDepthRT->GetSubresource(), mip, slice);

                rpDesc.Depth.Texture = static_cast<RGTexture*>(node->GetResource())->GetTexture();
                rpDesc.Depth.DepthLoadOp = m_pDepthRT->GetDepthLoadOp();
                rpDesc.Depth.StencilLoadOp = m_pDepthRT->GetStencilLoadOp();
                rpDesc.Depth.DepthStoreOp = node->IsCulled() ? RHI::ERHIRenderPassStoreOp::DontCare : RHI::ERHIRenderPassStoreOp::Store;
                rpDesc.Depth.StencilStoreOp = node->IsCulled() ? RHI::ERHIRenderPassStoreOp::DontCare : RHI::ERHIRenderPassStoreOp::Store;
                rpDesc.Depth.ClearDepth = m_pDepthRT->GetClearDepth();
                rpDesc.Depth.ClearStencil = m_pDepthRT->GetClearStencil();
                rpDesc.Depth.MipSlice = mip;
                rpDesc.Depth.ArraySlice = slice;
                rpDesc.Depth.bReadOnly = m_pDepthRT->IsReadOnly();
            }
            pCmdList->BeginRenderPass(rpDesc);
        }
    }

    void RenderGraphPassBase::End(RHI::RHICommandList *pCmdList)
    {
        if (HasRHIRenderPass())
        {
            pCmdList->EndRenderPass();
        }
    }

    bool RenderGraphPassBase::HasRHIRenderPass() const
    {
        for (int i = 0; i < RHI::RHI_MAX_COLOR_ATTACHMENT_COUNT; i++)
        {
            if (m_pColorRT[i] != nullptr)
            {
                return true;
            }
        }
        return m_pDepthRT != nullptr;
    }
}