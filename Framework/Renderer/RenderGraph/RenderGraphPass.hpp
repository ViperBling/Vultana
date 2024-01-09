#pragma once

#include "DAG.hpp"
#include "RHI/RHIPCH.hpp"

namespace Renderer
{
    enum class ERenderPassType
    {
        Graphics,
        Compute,
        AsyncCompute,
        Copy,
    };

    struct RenderGraphAsyncResolveContext
    {
        std::vector<DAGNodeID> ComputeQueuePasses;
        std::vector<DAGNodeID> PreGraphicsQueuePasses;
        std::vector<DAGNodeID> PostGraphicsQueuePasses;
        uint64_t ComputeFence;
        uint64_t GraphicsFence;
    };

    struct RenderGraphPassExecuteContext
    {
        RHI::RHICommandList* GraphicsCommandList;
        RHI::RHICommandList* ComputeCommandList;
        RHI::RHIFence* ComputeQueueFence;
        RHI::RHIFence* GraphicsQueueFence;

        uint64_t InitialComputeFenceValue;
        uint64_t LastSignaledComputeFenceValue;
        uint64_t InitialGraphicsFenceValue;
        uint64_t LastSignaledGraphicsFenceValue;
    };

    class RenderGraphPassBase : public DAGNode
    {
    public:
        RenderGraphPassBase(const std::string& name, ERenderPassType type, DirectedAcyclicGraph& graph);

        void ResolveBarriers(const DirectedAcyclicGraph& graph);
        void ResolveAsyncCompute(const DirectedAcyclicGraph& graph, RenderGraphAsyncResolveContext& context);
        void Execute(const DirectedAcyclicGraph& graph, RenderGraphPassExecuteContext& context);

        void BeginEvent(const std::string& name);


    private:


    private:
        
        
    };
    
}