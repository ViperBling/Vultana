#include "DAG.hpp"

#include <cassert>
#include <sstream>
#include <algorithm>

namespace RG
{
    DAGEdge::DAGEdge(DirectedAcyclicGraph &graph, DAGNode *from, DAGNode *to)
        : m_FromNode(from->GetID())
        , m_ToNode(to->GetID())
    {
        assert(graph.GetNode(m_FromNode) == from);
        assert(graph.GetNode(m_ToNode) == to);

        graph.RegisterEdge(this);
    }

    DAGNode::DAGNode(DirectedAcyclicGraph &graph)
    {
        m_ID = graph.GenerateNodeID();
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
        for (size_t i = 0; i < m_Edges.size(); ++i)
        {
            if (m_Edges[i]->m_FromNode == from && m_Edges[i]->m_ToNode == to) { return m_Edges[i]; }
        }
        return nullptr;
    }

    void DirectedAcyclicGraph::RegisterNode(DAGNode *node)
    {
        assert(node->GetID() == m_Nodes.size());
        m_Nodes.push_back(node);
    }

    void DirectedAcyclicGraph::RegisterEdge(DAGEdge * edge)
    {
        m_Edges.push_back(edge);
    }

    void DirectedAcyclicGraph::Clear()
    {
        m_Edges.clear();
        m_Nodes.clear();
    }

    void DirectedAcyclicGraph::Cull()
    {
        // 遍历图中的边，更新节点的引用计数
        for (size_t i = 0; i < m_Edges.size(); ++i)
        {
            DAGEdge* edge = m_Edges[i];
            DAGNode* node = m_Nodes[edge->m_FromNode];
            node->m_RefCount++;
        }

        // 遍历图中的节点，如果引用计数为0，入栈准备删除
        eastl::vector<DAGNode*> nodesStack;
        for (size_t i = 0; i < m_Nodes.size(); ++i)
        {
            if (m_Nodes[i]->GetRefCount() == 0)
            {
                nodesStack.push_back(m_Nodes[i]);
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
                DAGNode* linkedNode = GetNode(incomingEdges[i]->m_FromNode);
                if (--linkedNode->m_RefCount == 0)
                {
                    nodesStack.push_back(linkedNode);
                }
            }
        }
    }

    bool DirectedAcyclicGraph::IsEdgeValid(const DAGEdge *edge) const
    {
        return !GetNode(edge->m_FromNode)->IsCulled() && !GetNode(edge->m_ToNode)->IsCulled();
    }

    void DirectedAcyclicGraph::GetIncomingEdges(const DAGNode *node, eastl::vector<DAGEdge *> &edges) const
    {
        edges.clear();

        for (size_t i = 0; i < m_Edges.size(); ++i)
        {
            if (m_Edges[i]->m_ToNode == node->GetID())
            {
                edges.push_back(m_Edges[i]);
            }
        }
    }

    void DirectedAcyclicGraph::GetOutgoingEdges(const DAGNode *node, eastl::vector<DAGEdge *> &edges) const
    {
        edges.clear();

        for (size_t i = 0; i < m_Edges.size(); ++i)
        {
            if (m_Edges[i]->m_FromNode == node->GetID())
            {
                edges.push_back(m_Edges[i]);
            }
        }
    }

    eastl::string DirectedAcyclicGraph::ExportGraphViz()
    {
        std::stringstream ssOut;

        ssOut << "digraph DAG {\n";
        ssOut << "  rankdir=LR;\n";
        ssOut << "  node [fontname=\"helvetica\", fontsize=10]\n\n";

        for (size_t i = 0; i < m_Nodes.size(); i++)
        {
            uint32_t id = m_Nodes[i]->GetID();
            eastl::string s = m_Nodes[i]->GraphVizify();
            ssOut << "  \"N" << id << "\" " << s.c_str() << "\n";
        }
        ssOut << "\n";
        for (size_t i = 0; i < m_Nodes.size(); i++)
        {
            DAGNode* node = m_Nodes[i];
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
                    DAGNode const* ref = GetNode((*first++)->m_ToNode);
                    ssOut << "N" << ref->GetID() << " ";
                }
                ssOut << "} [color=" << s.c_str() << "2]\n";
            }

            if (first != edges.end())
            {
                ssOut << "  N" << id << " -> { ";
                while (first != edges.end())
                {
                    DAGNode const* ref = GetNode((*first++)->m_ToNode);
                    ssOut << "N" << ref->GetID() << " ";
                }
                ssOut << "} [color=" << s.c_str() << "4 style=dashed]\n";
            }
        }
        ssOut << "}" << std::endl;

        return ssOut.str().c_str();
    }

} // namespace Vultana::Renderer
