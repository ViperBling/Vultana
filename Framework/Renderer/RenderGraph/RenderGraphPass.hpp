#pragma once

#include "DAG.hpp"
#include "RHI/RHI.hpp"

#include <functional>

namespace Renderer
{
    class FRendererBase;
}

namespace RG
{
    class FRenderGraph;
    class FRenderGraphResource;
    class FRGEdgeColorAttachment;
    class FRGEdgeDepthAttachment;

    enum class RenderPassType
    {
        Graphics,
        Compute,
        AsyncCompute,
        Copy
    };

    struct FRenderGraphAsyncResolveContext
    {
        eastl::vector<DAGNodeID> ComputeQueuePasses;
        eastl::vector<DAGNodeID> PreGraphicsQueuePasses;
        eastl::vector<DAGNodeID> PostGraphicsQueuePasses;
        uint64_t ComputeFence = 0;
        uint64_t GraphicsFence = 0;
    };

    struct FRenderGraphPassExecuteContext
    {
        Renderer::FRendererBase* pRenderer;
        RHI::FRHICommandList* GraphicsCmdList;
        RHI::FRHICommandList* ComputeCmdList;
        RHI::FRHIFence* GraphicsFence;
        RHI::FRHIFence* ComputeFence;

        uint64_t InitialGraphicsFenceValue;
        uint64_t LastSignalGraphicsFenceValue;

        uint64_t InitialComputeFenceValue;;
        uint64_t LastSignalComputeFenceValue;
    };

    class FRenderGraphPassBase : public FDAGNode
    {
    public:
        FRenderGraphPassBase(const eastl::string& name, RenderPassType type, FDirectedAcyclicGraph& graph);
        
        void ResolveBarriers(const FDirectedAcyclicGraph& graph);
        void ResolveAsyncComputeBarrier(const FDirectedAcyclicGraph& graph, FRenderGraphAsyncResolveContext& context);
        void Execute(const FRenderGraph& graph, FRenderGraphPassExecuteContext& context);

        void BeginEvent(const eastl::string& name) { m_EventNames.push_back(name); }
        void EndEvent() { m_EndEventNum++; }

        RenderPassType GetType() const { return m_Type; }
        DAGNodeID GetWaitGraphicsPass() const { return m_WaitGraphicsPass; }
        DAGNodeID GetSignalGraphicsPass() const { return m_SignalGraphicsPass; }

        virtual eastl::string GetGraphVizName() const override { return m_Name; }
        virtual const char* GetGraphVizColor() const override { return !IsCulled() ? "darkgoldenrod1" : "darkgoldenrod4"; }

    private:
        void Begin(const FRenderGraph& graph, RHI::FRHICommandList* pCmdList);
        void End(RHI::FRHICommandList* pCmdList);

        bool HasRHIRenderPass() const;

        virtual void ExecuteImpl(RHI::FRHICommandList* pCmdList) = 0;
    
    protected:
        eastl::string m_Name;
        RenderPassType m_Type;

        eastl::vector<eastl::string> m_EventNames;
        uint32_t m_EndEventNum = 0;

        struct FResourceBarrier
        {
            FRenderGraphResource* Resource;
            uint32_t Subresource;
            RHI::ERHIAccessFlags OldState;
            RHI::ERHIAccessFlags NewState;
        };
        eastl::vector<FResourceBarrier> m_ResourceBarriers;

        struct FAliasDiscardBarrier
        {
            RHI::FRHIResource* Resource;
            RHI::ERHIAccessFlags AccessBefore;
            RHI::ERHIAccessFlags AccessAfter;
        };
        eastl::vector<FAliasDiscardBarrier> m_AliasDiscardBarriers;

        FRGEdgeColorAttachment* m_pColorRT[RHI::RHI_MAX_COLOR_ATTACHMENT_COUNT] = {};
        FRGEdgeDepthAttachment* m_pDepthRT = nullptr;

        DAGNodeID m_WaitGraphicsPass = UINT32_MAX;
        DAGNodeID m_SignalGraphicsPass = UINT32_MAX;

        uint64_t m_SignalValue = -1;
        uint64_t m_WaitValue = -1;
    };

    template<class T>
    class TRenderGraphPass : public FRenderGraphPassBase
    {
    public:
        TRenderGraphPass(const eastl::string& name, RenderPassType type, FDirectedAcyclicGraph& graph, const eastl::function<void(const T&, RHI::FRHICommandList*)>& execute)
            : FRenderGraphPassBase(name, type, graph)
        {
            m_Execute = execute;
        }

        T& GetData() { return m_Parameters; }
        T const* operator->() { return &GetData(); }
    
    private:
        void ExecuteImpl(RHI::FRHICommandList* pCmdList) override
        {
            m_Execute(m_Parameters, pCmdList);
        }
    
    protected:
        T m_Parameters;
        eastl::function<void(const T&, RHI::FRHICommandList*)> m_Execute;
    };
}