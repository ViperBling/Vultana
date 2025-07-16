#pragma once

#include "DAG.hpp"
#include "RHI/RHI.hpp"

#include <functional>

namespace Renderer
{
    class RendererBase;
}

namespace RG
{
    class RenderGraph;
    class RenderGraphResource;
    class RGEdgeColorAttachment;
    class RGEdgeDepthAttachment;

    enum class RenderPassType
    {
        Graphics,
        Compute,
        AsyncCompute,
        Copy
    };

    struct RenderGraphAsyncResolveContext
    {
        eastl::vector<DAGNodeID> ComputeQueuePasses;
        eastl::vector<DAGNodeID> PreGraphicsQueuePasses;
        eastl::vector<DAGNodeID> PostGraphicsQueuePasses;
        uint64_t ComputeFence = 0;
        uint64_t GraphicsFence = 0;
    };

    struct RenderGraphPassExecuteContext
    {
        Renderer::RendererBase* pRenderer;
        RHI::RHICommandList* GraphicsCmdList;
        RHI::RHICommandList* ComputeCmdList;
        RHI::RHIFence* GraphicsFence;
        RHI::RHIFence* ComputeFence;

        uint64_t InitialGraphicsFenceValue;
        uint64_t LastSignalGraphicsFenceValue;

        uint64_t InitialComputeFenceValue;;
        uint64_t LastSignalComputeFenceValue;
    };

    class RenderGraphPassBase : public DAGNode
    {
    public:
        RenderGraphPassBase(const eastl::string& name, RenderPassType type, DirectedAcyclicGraph& graph);
        
        void ResolveBarriers(const DirectedAcyclicGraph& graph);
        void ResolveAsyncComputeBarrier(const DirectedAcyclicGraph& graph, RenderGraphAsyncResolveContext& context);
        void Execute(const RenderGraph& graph, RenderGraphPassExecuteContext& context);

        void BeginEvent(const eastl::string& name) { m_EventNames.push_back(name); }
        void EndEvent() { m_EndEventNum++; }

        RenderPassType GetType() const { return m_Type; }
        DAGNodeID GetWaitGraphicsPass() const { return m_WaitGraphicsPass; }
        DAGNodeID GetSignalGraphicsPass() const { return m_SignalGraphicsPass; }

        virtual eastl::string GetGraphVizName() const override { return m_Name; }
        virtual const char* GetGraphVizColor() const override { return !IsCulled() ? "darkgoldenrod1" : "darkgoldenrod4"; }

    private:
        void Begin(const RenderGraph& graph, RHI::RHICommandList* pCmdList);
        void End(RHI::RHICommandList* pCmdList);

        bool HasRHIRenderPass() const;

        virtual void ExecuteImpl(RHI::RHICommandList* pCmdList) = 0;
    
    protected:
        eastl::string m_Name;
        RenderPassType m_Type;

        eastl::vector<eastl::string> m_EventNames;
        uint32_t m_EndEventNum = 0;

        struct ResourceBarrier
        {
            RenderGraphResource* Resource;
            uint32_t Subresource;
            RHI::ERHIAccessFlags OldState;
            RHI::ERHIAccessFlags NewState;
        };
        eastl::vector<ResourceBarrier> m_ResourceBarriers;

        struct AliasDiscardBarrier
        {
            RHI::RHIResource* Resource;
            RHI::ERHIAccessFlags AccessBefore;
            RHI::ERHIAccessFlags AccessAfter;
        };
        eastl::vector<AliasDiscardBarrier> m_AliasDiscardBarriers;

        RGEdgeColorAttachment* m_pColorRT[RHI::RHI_MAX_COLOR_ATTACHMENT_COUNT] = {};
        RGEdgeDepthAttachment* m_pDepthRT = nullptr;

        DAGNodeID m_WaitGraphicsPass = UINT32_MAX;
        DAGNodeID m_SignalGraphicsPass = UINT32_MAX;

        uint64_t m_SignalValue = -1;
        uint64_t m_WaitValue = -1;
    };

    template<class T>
    class RenderGraphPass : public RenderGraphPassBase
    {
    public:
        RenderGraphPass(const eastl::string& name, RenderPassType type, DirectedAcyclicGraph& graph, const eastl::function<void(const T&, RHI::RHICommandList*)>& execute)
            : RenderGraphPassBase(name, type, graph)
        {
            m_Execute = execute;
        }

        T& GetData() { return m_Parameters; }
        T const* operator->() { return &GetData(); }
    
    private:
        void ExecuteImpl(RHI::RHICommandList* pCmdList) override
        {
            m_Execute(m_Parameters, pCmdList);
        }
    
    protected:
        T m_Parameters;
        eastl::function<void(const T&, RHI::RHICommandList*)> m_Execute;
    };
}