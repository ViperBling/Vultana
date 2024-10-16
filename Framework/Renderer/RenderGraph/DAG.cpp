#include "DAG.hpp"

#include <cassert>

namespace RenderGraph
{
    DAGEdge::DAGEdge(DirectedAcyclicGraph &graph, DAGNode *from, DAGNode *to)
        : mFromNode(from->GetID())
        , mToNode(to->GetID())
    {
        assert(graph.GetNode(mFromNode) == from);
        assert(graph.GetNode(mToNode) == to);

        graph.RegisterEdge(this);
    }

    DAGNode::DAGNode(DirectedAcyclicGraph &graph)
    {
        mID = graph.GenerateNodeID();
        graph.RegisterNode(this);
    }

    DAGEdge* DirectedAcyclicGraph::GetEdge(DAGNodeID from, DAGNodeID to) const
    {
        for (size_t i = 0; i < mEdges.size(); ++i)
        {
            if (mEdges[i]->mFromNode == from && mEdges[i]->mToNode == to) { return mEdges[i]; }
        }
        return nullptr;
    }

    void DirectedAcyclicGraph::RegisterNode(DAGNode *node)
    {
        assert(node->GetID() == mNodes.size());
        mNodes.push_back(node);
    }

    void DirectedAcyclicGraph::RegisterEdge(DAGEdge * edge)
    {
        mEdges.push_back(edge);
    }

    void DirectedAcyclicGraph::Clear()
    {
        mEdges.clear();
        mNodes.clear();
    }

    void DirectedAcyclicGraph::Cull()
    {
        // 遍历图中的边，更新节点的引用计数
        for (size_t i = 0; i < mEdges.size(); ++i)
        {
            DAGEdge* edge = mEdges[i];
            DAGNode* node = mNodes[edge->mFromNode];
            node->mRefCount++;
        }

        // 遍历图中的节点，如果引用计数为0，入栈准备删除
        std::vector<DAGNode*> nodesStack;
        for (size_t i = 0; i < mNodes.size(); ++i)
        {
            if (mNodes[i]->GetRefCount() == 0)
            {
                nodesStack.push_back(mNodes[i]);
            }
        }

        while (!nodesStack.empty())
        {
            DAGNode* node = nodesStack.back();
            nodesStack.pop_back();

            std::vector<DAGEdge*> incomingEdges;
            GetIncomingEdges(node, incomingEdges);

            for (size_t i = 0; i < incomingEdges.size(); ++i)
            {
                DAGNode* linkedNode = GetNode(incomingEdges[i]->mFromNode);
                if (--linkedNode->mRefCount == 0)
                {
                    nodesStack.push_back(linkedNode);
                }
            }
        }
    }

    bool DirectedAcyclicGraph::IsEdgeValid(const DAGEdge *edge) const
    {
        return !GetNode(edge->mFromNode)->IsCulled() && !GetNode(edge->mToNode)->IsCulled();
    }

    void DirectedAcyclicGraph::GetIncomingEdges(const DAGNode *node, std::vector<DAGEdge *> &edges) const
    {
        edges.clear();

        for (size_t i = 0; i < mEdges.size(); ++i)
        {
            if (mEdges[i]->mToNode == node->GetID())
            {
                edges.push_back(mEdges[i]);
            }
        }
    }

    void DirectedAcyclicGraph::GetOutgoingEdges(const DAGNode *node, std::vector<DAGEdge *> &edges) const
    {
        edges.clear();

        for (size_t i = 0; i < mEdges.size(); ++i)
        {
            if (mEdges[i]->mFromNode == node->GetID())
            {
                edges.push_back(mEdges[i]);
            }
        }
    }

} // namespace Vultana::Renderer
