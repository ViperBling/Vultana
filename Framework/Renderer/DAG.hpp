#pragma once

#include <vector>
#include <string>

namespace Renderer
{
    using DAGNodeID = uint32_t;

    class DirectedAcyclicGraph;
    class DAGNode;

    class DAGEdge
    {
        friend class DirectedAcyclicGraph;

    public:
        DAGEdge(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to);
        virtual ~DAGEdge() {}

        DAGNodeID GetFromNode() const { return mFromNode; }
        DAGNodeID GetToNode() const { return mToNode; }

    private:
        const DAGNodeID mFromNode;
        const DAGNodeID mToNode;
    };

    class DAGNode
    {
        friend class DirectedAcyclicGraph;

    public:
        DAGNode(DirectedAcyclicGraph& graph, const std::string& name);
        virtual ~DAGNode() {}

        DAGNodeID GetID() const { return mID; }
        const std::string& GetName() const { return mName; }

        void MakeTarget() { mRefCount = TARGET; }

        bool IsTarget() const { return mRefCount >= TARGET; }
        bool IsCulled() const { return mRefCount == 0; }
        uint32_t GetRefCount() const { return IsTarget() ? 1 : mRefCount; }

    private:
        DAGNodeID mID;
        uint32_t mRefCount = 0;
        std::string mName;

        static const uint32_t TARGET = 0x80000000u;
    };

    class DirectedAcyclicGraph
    {
    public:
        DAGNodeID GenerateNodeID() { return (DAGNodeID)mNodes.size(); }
        DAGNode* GetNode(DAGNodeID id) const { return mNodes[id]; }
        DAGEdge* GetEdge(DAGNodeID from, DAGNodeID to) const;

        void RegisterNode(DAGNode* node);
        void RegisterEdge(DAGEdge* edge);

        void Clear();
        void Cull();
        bool IsEdgeValid(const DAGEdge* edge) const;

        void GetIncomingEdges(const DAGNode* node, std::vector<DAGEdge*>& edges) const;
        void GetOutgoingEdges(const DAGNode* node, std::vector<DAGEdge*>& edges) const;

        //dot.exe -Tpng -O file
        // std::string ExportGraphviz();

    private:
        std::vector<DAGNode*> mNodes;
        std::vector<DAGEdge*> mEdges;
    };
}