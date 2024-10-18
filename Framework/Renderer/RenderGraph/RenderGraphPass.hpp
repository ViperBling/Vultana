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
        std::vector<DAGNodeID> ComputeQueuePasses;
        std::vector<DAGNodeID> PreGraphicsQueuePasses;
        std::vector<DAGNodeID> PostGraphicsQueuePasses;
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
        RenderGraphPassBase(const std::string& name, RenderPassType type, DirectedAcyclicGraph& graph);
        
        void ResolveBarriers(const DirectedAcyclicGraph& graph);
        void ResolveAsyncComputeBarrier(const DirectedAcyclicGraph& graph, RenderGraphAsyncResolveContext& context);
        void Execute(const RenderGraph& graph, RenderGraphPassExecuteContext& context);

        void BeginEvent(const std::string& name) { mEventNames.push_back(name); }
        void EndEvent() { mEndEventNum++; }

        RenderPassType GetType() const { return mType; }
        DAGNodeID GetWaitGraphicsPass() const { return mWaitGraphicsPass; }
        DAGNodeID GetSignalGraphicsPass() const { return mSignalGraphicsPass; }

    private:
        void Begin(const RenderGraph& graph, RHI::RHICommandList* pCmdList);
        void End(RHI::RHICommandList* pCmdList);

        bool HasRHIRenderPass() const;

        virtual void ExecuteImpl(RHI::RHICommandList* pCmdList) = 0;
    
    private:
        std::string mName;
        RenderPassType mType;

        std::vector<std::string> mEventNames;
        uint32_t mEndEventNum = 0;

        struct ResourceBarrier
        {
            RenderGraphResource* Resource;
            uint32_t Subresource;
            RHI::ERHIAccessFlags OldState;
            RHI::ERHIAccessFlags NewState;
        };
        std::vector<ResourceBarrier> mResourceBarriers;

        struct AliasDiscardBarrier
        {
            RHI::RHIResource* Resource;
            RHI::ERHIAccessFlags AccessBefore;
            RHI::ERHIAccessFlags AccessAfter;
        };
        std::vector<AliasDiscardBarrier> mAliasDiscardBarriers;

        RGEdgeColorAttachment* mpColorRT[RHI::RHI_MAX_COLOR_ATTACHMENT_COUNT] = {};
        RGEdgeDepthAttachment* mpDepthRT = nullptr;

        DAGNodeID mWaitGraphicsPass = UINT32_MAX;
        DAGNodeID mSignalGraphicsPass = UINT32_MAX;

        uint64_t mSignalValue = -1;
        uint64_t mWaitValue = -1;
    };

    template<class T>
    class RenderGraphPass : public RenderGraphPassBase
    {
    public:
        RenderGraphPass(const std::string& name, RenderPassType type, DirectedAcyclicGraph& graph, const std::function<void(const T&, RHI::RHICommandList*)>& execute)
            : RenderGraphPassBase(name, type, graph)
        {
            mExecute = execute;
        }

        T& GetData() { return mParameters; }
        T const* operator->() { return &GetData(); }
    
    private:
        void ExecuteImpl(RHI::RHICommandList* pCmdList) override
        {
            mExecute(mParameters, pCmdList);
        }
    
    protected:
        T mParameters;
        std::function<void(const T&, RHI::RHICommandList*)> mExecute;
    };
}