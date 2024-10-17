#include "RenderGraphPass.hpp"

namespace RG
{
    RenderGraphPassBase::RenderGraphPassBase(const std::string &name, RenderPassType type, DirectedAcyclicGraph &graph)
        : DAGNode(graph)
    {
        mName = name;
        mType = type;
    }

    void RenderGraphPassBase::ResolveBarrier(const DirectedAcyclicGraph &graph)
    {
    }

    void RenderGraphPassBase::ResolveAsyncComputeBarrier(const DirectedAcyclicGraph &graph, RenderGraphAsyncResolveContext &context)
    {
    }

    void RenderGraphPassBase::Execute(const RenderGraph &graph, RenderGraphPassExecuteContext &context)
    {
    }

    void RenderGraphPassBase::Begin(const RenderGraph &graph, RHI::RHICommandList *pCmdList)
    {
    }

    void RenderGraphPassBase::End(RHI::RHICommandList *pCmdList)
    {
    }

    bool RenderGraphPassBase::HasRHIRenderPass() const
    {
        return false;
    }
}