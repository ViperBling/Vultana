#pragma once

#include "RenderGraph.hpp"
#include "RenderGraphBuilder.hpp"

namespace RG
{
    class FRenderGraphEdge : public FDAGEdge
    {
    public:
        FRenderGraphEdge(FDirectedAcyclicGraph& graph, FDAGNode* from, FDAGNode* to, RHI::ERHIAccessFlags usage, uint32_t subresource)
            : FDAGEdge(graph, from, to)
        {
            m_Usage = usage;
            m_Subresource = subresource;
        }

        RHI::ERHIAccessFlags GetUsage() const { return m_Usage; }
        uint32_t GetSubresource() const { return m_Subresource; }
    
    private:
        RHI::ERHIAccessFlags m_Usage;
        uint32_t m_Subresource;
    };

    class FRenderGraphResourceNode : public FDAGNode
    {
    public:
        FRenderGraphResourceNode(FDirectedAcyclicGraph& graph, FRenderGraphResource* resource, uint32_t version)
            : FDAGNode(graph)
            , m_Graph(graph)
        {
            m_pResource = resource;
            m_Version = version;
        }
    
        FRenderGraphResource* GetResource() const { return m_pResource; }
        uint32_t GetVersion() const { return m_Version; }

        virtual eastl::string GetGraphVizName() const override 
        {
            eastl::string str = m_pResource->GetName();
            str.append("\nversion:");
            str.append(eastl::to_string(m_Version));
            if (m_Version > 0)
            {
                eastl::vector<FDAGEdge*> incomingEdges;
                m_Graph.GetIncomingEdges(this, incomingEdges);
                assert(incomingEdges.size() == 1);
                uint32_t subresource = ((FRenderGraphEdge*)incomingEdges[0])->GetSubresource();
                str.append("\nsubresource:");
                str.append(eastl::to_string(subresource));
            }
            return str;
        }
        virtual const char* GetGraphVizColor() const override { return !IsCulled() ? "lightskyblue1" : "lightskyblue4"; }
        virtual const char* GetGraphVizShape() const override { return "ellipse"; }
    
    private:
        FRenderGraphResource* m_pResource = nullptr;
        uint32_t m_Version;
        FDirectedAcyclicGraph& m_Graph;
    };

    class FRGEdgeColorAttachment : public FRenderGraphEdge
    {
    public:
        FRGEdgeColorAttachment(FDirectedAcyclicGraph& graph, FDAGNode* from, FDAGNode* to, RHI::ERHIAccessFlags usage, uint32_t subresource, uint32_t colorIndex, RHI::ERHIRenderPassLoadOp loadOp, const float4& clearColor)
            : FRenderGraphEdge(graph, from, to, usage, subresource)
        {
            m_ColorIndex = colorIndex;
            m_LoadOp = loadOp;
            m_ClearColor[0] = clearColor[0];
            m_ClearColor[1] = clearColor[1];
            m_ClearColor[2] = clearColor[2];
            m_ClearColor[3] = clearColor[3];
        }

        uint32_t GetColorIndex() const { return m_ColorIndex; }
        RHI::ERHIRenderPassLoadOp GetLoadOp() const { return m_LoadOp; }
        const float* GetClearColor() const { return m_ClearColor; }
    
    private:
        uint32_t m_ColorIndex;
        RHI::ERHIRenderPassLoadOp m_LoadOp;
        float m_ClearColor[4] = {};
    };

    class FRGEdgeDepthAttachment : public FRenderGraphEdge
    {
    public:
        FRGEdgeDepthAttachment(FDirectedAcyclicGraph& graph, FDAGNode* from, FDAGNode* to, RHI::ERHIAccessFlags usage, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil)
            : FRenderGraphEdge(graph, from, to, usage, subresource)
        {
            m_DepthLoadOp = depthLoadOp;
            m_StencilLoadOp = stencilLoadOp;
            m_ClearDepth = clearDepth;
            m_ClearStencil = clearStencil;
            m_bReadOnly = (usage & RHI::RHIAccessDSVReadOnly) ? true : false;
        }

        RHI::ERHIRenderPassLoadOp GetDepthLoadOp() const { return m_DepthLoadOp; }
        RHI::ERHIRenderPassLoadOp GetStencilLoadOp() const { return m_StencilLoadOp; }
        float GetClearDepth() const { return m_ClearDepth; }
        uint32_t GetClearStencil() const { return m_ClearStencil; }
        bool IsReadOnly() const { return m_bReadOnly; }
    
    private:
        RHI::ERHIRenderPassLoadOp m_DepthLoadOp;
        RHI::ERHIRenderPassLoadOp m_StencilLoadOp;
        float m_ClearDepth;
        uint32_t m_ClearStencil;
        bool m_bReadOnly;
    };

    template<typename T>
    void ClassFinalizer(void* p)
    {
        ((T*)p)->~T();
    }

    template <typename Data, typename Setup, typename Execution>
    inline TRenderGraphPass<Data> &FRenderGraph::AddPass(const eastl::string &name, RenderPassType type, const Setup &setup, const Execution &execution)
    {
        auto pass = Allocate<TRenderGraphPass<Data>>(name, type, m_Graph, execution);
        for (size_t i = 0; i < m_EventNames.size(); i++)
        {
            pass->BeginEvent(m_EventNames[i]);
        }
        m_EventNames.clear();

        FRGBuilder builder(this, pass);
        setup(pass->GetData(), builder);

        m_Passes.push_back(pass);

        return *pass;
    }

    template <typename T, typename... ArgsT>
    inline T *FRenderGraph::Allocate(ArgsT &&...args)
    {
        T* p = (T*)m_Allocator.Allocate(sizeof(T));
        new (p) T(args...);

        FObjFinalizer finalizer;
        finalizer.Object = p;
        finalizer.Finalizer = &ClassFinalizer<T>;
        m_ObjFinalizers.push_back(finalizer);
        return p;
    }

    template<typename T, typename... ArgsT>
    inline T* FRenderGraph::AllocatePOD(ArgsT&&... args)
    {
        T* p = (T*)m_Allocator.Allocate(sizeof(T));
        new (p) T(args...);
        return p;
    }

    template <typename Resource>
    inline FRGHandle FRenderGraph::Create(const typename Resource::Desc &desc, const eastl::string &name)
    {
        auto resource = Allocate<Resource>(m_ResourceAllocator, name, desc);
        auto node = AllocatePOD<FRenderGraphResourceNode>(m_Graph, resource, 0);

        FRGHandle handle;
        handle.Index = (uint16_t)m_Resources.size();
        handle.Node = (uint16_t)m_ResourceNodes.size();

        m_Resources.push_back(resource);
        m_ResourceNodes.push_back(node);

        return handle;
    }
}