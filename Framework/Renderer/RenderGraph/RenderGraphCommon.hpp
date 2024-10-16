#pragma once

#include "RenderGraph.hpp"
#include "RenderGraphBuilder.hpp"

namespace RenderGraph
{
    class RenderGraphEdge : public DAGEdge
    {
    public:
        RenderGraphEdge(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to, RHI::ERHIAccessFlags usage, uint32_t subresource)
            : DAGEdge(graph, from, to)
        {
            mUsage = usage;
            mSubresource = subresource;
        }

        RHI::ERHIAccessFlags GetUsage() const { return mUsage; }
        uint32_t GetSubresource() const { return mSubresource; }
    
    private:
        RHI::ERHIAccessFlags mUsage;
        uint32_t mSubresource;
    };

    class RenderGraphResourceNode : public DAGNode
    {
    public:
        RenderGraphResourceNode(DirectedAcyclicGraph& graph, RenderGraphResource* resource, uint32_t version)
            : DAGNode(graph)
            , mGraph(graph)
        {
            mpResource = resource;
            mVersion = version;
        }
    
    
    private:
        RenderGraphResource* mpResource = nullptr;
        uint32_t mVersion;
        DirectedAcyclicGraph& mGraph;
    };
}