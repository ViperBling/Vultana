#pragma once

#include <EASTL/vector.h>
#include <EASTL/string.h>

namespace RG
{
    using DAGNodeID = uint32_t;

    class FDirectedAcyclicGraph;
    class FDAGNode;

    class FDAGEdge
    {
        friend class FDirectedAcyclicGraph;

    public:
        FDAGEdge(FDirectedAcyclicGraph& graph, FDAGNode* from, FDAGNode* to);
        virtual ~FDAGEdge() {}

        DAGNodeID GetFromNode() const { return m_FromNode; }
        DAGNodeID GetToNode() const { return m_ToNode; }

    private:
        const DAGNodeID m_FromNode;
        const DAGNodeID m_ToNode;
    };

    class FDAGNode
    {
        friend class FDirectedAcyclicGraph;

    public:
        FDAGNode(FDirectedAcyclicGraph& graph);
        virtual ~FDAGNode() {}

        DAGNodeID GetID() const { return m_ID; }

        void MakeTarget() { m_RefCount = TARGET; }

        bool IsTarget() const { return m_RefCount >= TARGET; }
        bool IsCulled() const { return m_RefCount == 0; }
        uint32_t GetRefCount() const { return IsTarget() ? 1 : m_RefCount; }

        virtual eastl::string GetGraphVizName() const { return "unknown"; }
        virtual const char* GetGraphVizColor() const { return !IsCulled() ? "Skyblue" : "Skyblue4"; }
        virtual const char* GetGraphVizEdgeColor() const { return "Darkolivegreen"; }
        virtual const char* GetGraphVizShape() const { return "rectangle"; }
        eastl::string GraphVizify() const;

    private:
        DAGNodeID m_ID;
        uint32_t m_RefCount = 0;

        static const uint32_t TARGET = 0x80000000u;
    };

    class FDirectedAcyclicGraph
    {
    public:
        DAGNodeID GenerateNodeID() { return (DAGNodeID)m_Nodes.size(); }
        FDAGNode* GetNode(DAGNodeID id) const { return m_Nodes[id]; }
        FDAGEdge* GetEdge(DAGNodeID from, DAGNodeID to) const;

        void RegisterNode(FDAGNode* node);
        void RegisterEdge(FDAGEdge* edge);

        void Clear();
        void Cull();
        bool IsEdgeValid(const FDAGEdge* edge) const;

        void GetIncomingEdges(const FDAGNode* node, eastl::vector<FDAGEdge*>& edges) const;
        void GetOutgoingEdges(const FDAGNode* node, eastl::vector<FDAGEdge*>& edges) const;

        eastl::string ExportGraphViz();

    private:
        eastl::vector<FDAGNode*> m_Nodes;
        eastl::vector<FDAGEdge*> m_Edges;
    };
}