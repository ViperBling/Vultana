#include "DAG.hpp"

#include <cassert>
#include <sstream>
#include <algorithm>

namespace RG
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

    eastl::string DAGNode::GraphVizify() const
    {
        eastl::string str;
        str.reserve(128);

        str.append("[label=\"");
        str.append(GetGraphVizName());
        str.append("\", style=filled, shape=");
        str.append(GetGraphVizShape());
        str.append(", fillcolor=");
        str.append(GetGraphVizColor());
        str.append("]");
        str.shrink_to_fit();

        return str;
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
        eastl::vector<DAGNode*> nodesStack;
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

            eastl::vector<DAGEdge*> incomingEdges;
            // 获取当前node的所有入边
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

    void DirectedAcyclicGraph::GetIncomingEdges(const DAGNode *node, eastl::vector<DAGEdge *> &edges) const
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

    void DirectedAcyclicGraph::GetOutgoingEdges(const DAGNode *node, eastl::vector<DAGEdge *> &edges) const
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

    eastl::string DirectedAcyclicGraph::ExportGraphViz()
    {
        std::stringstream ssOut;

        ssOut << "digraph DAG {\n";
        ssOut << "  rankdir=LR;\n";
        ssOut << "  node [fontname=\"helvetica\", fontsize=10]\n\n";

        for (size_t i = 0; i < mNodes.size(); i++)
        {
            uint32_t id = mNodes[i]->GetID();
            eastl::string s = mNodes[i]->GraphVizify();
            ssOut << "  \"N" << id << "\" " << s.c_str() << "\n";
        }
        ssOut << "\n";
        for (size_t i = 0; i < mNodes.size(); i++)
        {
            DAGNode* node = mNodes[i];
            uint32_t id = node->GetID();

            eastl::vector<DAGEdge*> edges;
            GetOutgoingEdges(node, edges);

            auto first = edges.begin();
            auto pos = std::partition(first, edges.end(), [this](auto const& edge) { return IsEdgeValid(edge); });
            
            eastl::string s = node->GetGraphVizEdgeColor();

            if (first != pos)
            {
                ssOut << "  N" << id << " -> { ";
                while (first != pos)
                {
                    DAGNode const* ref = GetNode((*first++)->mToNode);
                    ssOut << "N" << ref->GetID() << " ";
                }
                ssOut << "} [color=" << s.c_str() << "2]\n";
            }

            if (first != edges.end())
            {
                ssOut << "  N" << id << " -> { ";
                while (first != edges.end())
                {
                    DAGNode const* ref = GetNode((*first++)->mToNode);
                    ssOut << "N" << ref->GetID() << " ";
                }
                ssOut << "} [color=" << s.c_str() << "4 style=dashed]\n";
            }
        }
        ssOut << "}" << std::endl;

        return ssOut.str().c_str();
    }

} // namespace Vultana::Renderer
