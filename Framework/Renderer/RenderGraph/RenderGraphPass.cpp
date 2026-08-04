#include "RenderGraphPass.hpp"
#include "RenderGraph.hpp"
#include "Renderer/RendererBase.hpp"

#include <algorithm>

namespace RG
{
    FRenderGraphPassBase::FRenderGraphPassBase(const eastl::string &name, RenderPassType type, FDirectedAcyclicGraph &graph)
        : FDAGNode(graph)
    {
        m_Name = name;
        m_Type = type;
    }

    void FRenderGraphPassBase::ResolveBarriers(const FDirectedAcyclicGraph &graph)
    {
        eastl::vector<FDAGEdge*> edges;
        eastl::vector<FDAGEdge*> resIncoming;
        eastl::vector<FDAGEdge*> resOutgoing;

        graph.GetIncomingEdges(this, edges);
        for (size_t i = 0; i < edges.size(); i++)
        {
            FRenderGraphEdge *edge = static_cast<FRenderGraphEdge *>(edges[i]);
            assert(edge->GetToNode() == this->GetID());

            FRenderGraphResourceNode* resourceNode = static_cast<FRenderGraphResourceNode*>(graph.GetNode(edge->GetFromNode()));
            FRenderGraphResource* resource = resourceNode->GetResource();

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
                    uint32_t subresource = ((FRenderGraphEdge*)resOutgoing[i])->GetSubresource();
                    DAGNodeID passID = resOutgoing[i]->GetToNode();
                    if (subresource == edge->GetSubresource() && passID < this->GetID() && !graph.GetNode(passID)->IsCulled())
                    {
                        oldState = ((FRenderGraphEdge*)resOutgoing[i])->GetUsage();
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
                    oldState = ((FRenderGraphEdge*)resIncoming[0])->GetUsage();
                }
            }
            
            bool isAliased = false;
            RHI::ERHIAccessFlags aliasState;

            if (resource->IsOverlapping() && resource->GetFirstPassID() == this->GetID())
            {
                RHI::FRHIResource* aliasedRes = resource->GetAliasedPrevResource(aliasState);
                if (aliasedRes)
                {
                    m_AliasDiscardBarriers.push_back({ aliasedRes, aliasState, newState | RHI::RHIAccessDiscard });
                    isAliased = true;
                }
            }
            if (oldState != newState || isAliased)
            {
                FResourceBarrier barrier;
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
            FRenderGraphEdge* edge = static_cast<FRenderGraphEdge*>(edges[i]);
            assert(edge->GetFromNode() == this->GetID());

            RHI::ERHIAccessFlags newState = edge->GetUsage();

            if (newState == RHI::RHIAccessRTV)
            {
                assert(dynamic_cast<FRGEdgeColorAttachment*>(edge) != nullptr);
                FRGEdgeColorAttachment* colorRT = static_cast<FRGEdgeColorAttachment*>(edge);
                m_pColorRT[colorRT->GetColorIndex()] = colorRT;
            }
            else if (newState == RHI::RHIAccessDSV || newState == RHI::RHIAccessDSVReadOnly)
            {
                assert(dynamic_cast<FRGEdgeDepthAttachment*>(edge) != nullptr);
                m_pDepthRT = static_cast<FRGEdgeDepthAttachment*>(edge);
            }
        }
    }

    void FRenderGraphPassBase::ResolveAsyncComputeBarrier(const FDirectedAcyclicGraph &graph, FRenderGraphAsyncResolveContext &context)
    {
        if (m_Type == RenderPassType::AsyncCompute)
        {
            eastl::vector<FDAGEdge*> edges;
            eastl::vector<FDAGEdge*> resIncoming;
            eastl::vector<FDAGEdge*> resOutgoing;

            graph.GetIncomingEdges(this, edges);
            for (size_t i = 0; i < edges.size(); i++)
            {
                FRenderGraphEdge* edge = static_cast<FRenderGraphEdge*>(edges[i]);
                assert(edge->GetToNode() == this->GetID());

                FRenderGraphResourceNode* resourceNode = static_cast<FRenderGraphResourceNode*>(graph.GetNode(edge->GetFromNode()));

                graph.GetIncomingEdges(resourceNode, resIncoming);
                assert(resIncoming.size() <= 1);

                if (!resIncoming.empty())
                {
                    FRenderGraphPassBase* prePass = static_cast<FRenderGraphPassBase*>(graph.GetNode(resIncoming[0]->GetFromNode()));
                    if (!prePass->IsCulled() && prePass->GetType() != RenderPassType::AsyncCompute)
                    {
                        context.PreGraphicsQueuePasses.push_back(prePass->GetID());
                    }
                }
            }

            graph.GetOutgoingEdges(this, edges);
            for (size_t i = 0; i < edges.size(); i++)
            {
                FRenderGraphEdge* edge = static_cast<FRenderGraphEdge*>(edges[i]);
                assert(edge->GetFromNode() == this->GetID());

                FRenderGraphResourceNode* resourceNode = static_cast<FRenderGraphResourceNode*>(graph.GetNode(edge->GetToNode()));

                graph.GetOutgoingEdges(resourceNode, resOutgoing);

                for (size_t j = 0; j < resOutgoing.size(); j++)
                {
                    FRenderGraphPassBase* postPass = static_cast<FRenderGraphPassBase*>(graph.GetNode(resOutgoing[j]->GetToNode()));
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

                    FRenderGraphPassBase* graphicsPassToWait = static_cast<FRenderGraphPassBase*>(graph.GetNode(graphicsPassToWaitID));
                    if (graphicsPassToWait->m_SignalValue == -1)
                    {
                        graphicsPassToWait->m_SignalValue = ++context.GraphicsFence;
                    }

                    FRenderGraphPassBase* computePass = static_cast<FRenderGraphPassBase*>(graph.GetNode(context.ComputeQueuePasses[0]));
                    computePass->m_WaitValue = graphicsPassToWait->m_SignalValue;

                    for (size_t i = 0; i < context.ComputeQueuePasses.size(); i++)
                    {
                        FRenderGraphPassBase* pass = static_cast<FRenderGraphPassBase*>(graph.GetNode(context.ComputeQueuePasses[i]));
                        pass->m_WaitGraphicsPass = graphicsPassToWaitID;
                    }
                }

                if (!context.PostGraphicsQueuePasses.empty())
                {
                    DAGNodeID graphicsPassToSignalID = *eastl::min_element(context.PostGraphicsQueuePasses.begin(), context.PostGraphicsQueuePasses.end());

                    FRenderGraphPassBase* computePass = static_cast<FRenderGraphPassBase*>(graph.GetNode(context.ComputeQueuePasses.back()));
                    if (computePass->m_SignalValue == -1)
                    {
                        computePass->m_SignalValue = ++context.ComputeFence;
                    }

                    FRenderGraphPassBase* graphicsPassToSignal = static_cast<FRenderGraphPassBase*>(graph.GetNode(graphicsPassToSignalID));
                    graphicsPassToSignal->m_WaitValue = computePass->m_SignalValue;

                    for (size_t i = 0; i < context.ComputeQueuePasses.size(); i++)
                    {
                        FRenderGraphPassBase* pass = static_cast<FRenderGraphPassBase*>(graph.GetNode(context.ComputeQueuePasses[i]));
                        pass->m_SignalGraphicsPass = graphicsPassToSignalID;
                    }
                }

                context.ComputeQueuePasses.clear();
                context.PreGraphicsQueuePasses.clear();
                context.PostGraphicsQueuePasses.clear();
            }
        }
    }

    void FRenderGraphPassBase::Execute(const FRenderGraph &graph, FRenderGraphPassExecuteContext &context)
    {
        RHI::FRHICommandList* pCmdList = m_Type == RenderPassType::AsyncCompute ? context.ComputeCmdList : context.GraphicsCmdList;

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

    void FRenderGraphPassBase::Begin(const FRenderGraph &graph, RHI::FRHICommandList *pCmdList)
    {
        for (size_t i = 0; i < m_AliasDiscardBarriers.size(); i++)
        {
            const FAliasDiscardBarrier& barrier = m_AliasDiscardBarriers[i];

            if (barrier.Resource->IsTexture())
            {
                pCmdList->TextureBarrier((RHI::FRHITexture*)barrier.Resource, RHI::RHI_ALL_SUB_RESOURCE, barrier.AccessBefore, barrier.AccessAfter);
            }
            else
            {
                pCmdList->BufferBarrier((RHI::FRHIBuffer*)barrier.Resource, barrier.AccessBefore, barrier.AccessAfter);
            }
        }

        for (size_t i = 0; i < m_ResourceBarriers.size(); i++)
        {
            const FResourceBarrier& barrier = m_ResourceBarriers[i];
            barrier.Resource->Barrier(pCmdList, barrier.Subresource, barrier.OldState, barrier.NewState);
        }

        if (HasRHIRenderPass())
        {
            RHI::FRHIRenderPassDesc rpDesc;

            for (int i = 0; i < RHI::RHI_MAX_COLOR_ATTACHMENT_COUNT; i++)
            {
                if (m_pColorRT[i] != nullptr)
                {
                    FRenderGraphResourceNode* node = static_cast<FRenderGraphResourceNode*>(graph.GetDAG().GetNode(m_pColorRT[i]->GetToNode()));
                    RHI::FRHITexture* texture = static_cast<FRGTexture*>(node->GetResource())->GetTexture();

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
                FRenderGraphResourceNode* node = static_cast<FRenderGraphResourceNode*>(graph.GetDAG().GetNode(m_pDepthRT->GetToNode()));
                RHI::FRHITexture* texture = static_cast<FRGTexture*>(node->GetResource())->GetTexture();

                uint32_t mip, slice;
                RHI::DecomposeSubresource(texture->GetDesc(), m_pDepthRT->GetSubresource(), mip, slice);

                rpDesc.Depth.Texture = static_cast<FRGTexture*>(node->GetResource())->GetTexture();
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

    void FRenderGraphPassBase::End(RHI::FRHICommandList *pCmdList)
    {
        if (HasRHIRenderPass())
        {
            pCmdList->EndRenderPass();
        }
    }

    bool FRenderGraphPassBase::HasRHIRenderPass() const
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