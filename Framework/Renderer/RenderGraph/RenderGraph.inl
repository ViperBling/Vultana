#pragma once

#include "RenderGraph.hpp"
#include "RenderGraphBuilder.hpp"

namespace RG
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
    
        RenderGraphResource* GetResource() const { return mpResource; }
        uint32_t GetVersion() const { return mVersion; }
    
    private:
        RenderGraphResource* mpResource = nullptr;
        uint32_t mVersion;
        DirectedAcyclicGraph& mGraph;
    };

    class RGEdgeColorAttachment : public RenderGraphEdge
    {
    public:
        RGEdgeColorAttachment(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to, RHI::ERHIAccessFlags usage, uint32_t subresource, uint32_t colorIndex, RHI::ERHIRenderPassLoadOp loadOp, const float4& clearColor)
            : RenderGraphEdge(graph, from, to, usage, subresource)
        {
            mColorIndex = colorIndex;
            mLoadOp = loadOp;
            mClearColor[0] = clearColor[0];
            mClearColor[1] = clearColor[1];
            mClearColor[2] = clearColor[2];
            mClearColor[3] = clearColor[3];
        }

        uint32_t GetColorIndex() const { return mColorIndex; }
        RHI::ERHIRenderPassLoadOp GetLoadOp() const { return mLoadOp; }
        const float* GetClearColor() const { return mClearColor; }
    
    private:
        uint32_t mColorIndex;
        RHI::ERHIRenderPassLoadOp mLoadOp;
        float mClearColor[4] = {};
    };

    class RGEdgeDepthAttachment : public RenderGraphEdge
    {
    public:
        RGEdgeDepthAttachment(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to, RHI::ERHIAccessFlags usage, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil)
            : RenderGraphEdge(graph, from, to, usage, subresource)
        {
            mDepthLoadOp = depthLoadOp;
            mStencilLoadOp = stencilLoadOp;
            mClearDepth = clearDepth;
            mClearStencil = clearStencil;
            mbReadOnly = (usage & RHI::RHIAccessDSVReadOnly) ? true : false;
        }

        RHI::ERHIRenderPassLoadOp GetDepthLoadOp() const { return mDepthLoadOp; }
        RHI::ERHIRenderPassLoadOp GetStencilLoadOp() const { return mStencilLoadOp; }
        float GetClearDepth() const { return mClearDepth; }
        uint32_t GetClearStencil() const { return mClearStencil; }
        bool IsReadOnly() const { return mbReadOnly; }
    
    private:
        RHI::ERHIRenderPassLoadOp mDepthLoadOp;
        RHI::ERHIRenderPassLoadOp mStencilLoadOp;
        float mClearDepth;
        uint32_t mClearStencil;
        bool mbReadOnly;
    };

    template<typename T>
    void ClassFinalizer(void* p)
    {
        ((T*)p)->~T();
    }

    template <typename Data, typename Setup, typename Execution>
    inline RenderGraphPass<Data> &RenderGraph::AddPass(const std::string &name, RenderPassType type, const Setup &setup, const Execution &execution)
    {
        auto pass = Allocate<RenderGraphPass<Data>>(name, type, mGraph, execution);
        for (size_t i = 0; i < mEventNames.size(); i++)
        {
            pass->BeginEvent(mEventNames[i]);
        }
        mEventNames.clear();

        RGBuilder builder(this, pass);
        setup(pass->GetData(), builder);

        mPasses.push_back(pass);

        return *pass;
    }

    template <typename T, typename... ArgsT>
    inline T *RenderGraph::Allocate(ArgsT &&...args)
    {
        T* p = (T*)mAllocator.Allocate(sizeof(T));
        new (p) T(args...);

        ObjFinalizer finalizer;
        finalizer.Object = p;
        finalizer.Finalizer = &ClassFinalizer<T>;
        mObjFinalizers.push_back(finalizer);
        return p;
    }

    template<typename T, typename... ArgsT>
    inline T* RenderGraph::AllocatePOD(ArgsT&&... args)
    {
        T* p = (T*)mAllocator.Allocate(sizeof(T));
        new (p) T(args...);
        return p;
    }

    template <typename Resource>
    inline RGHandle RenderGraph::Create(const typename Resource::Desc &desc, const std::string &name)
    {
        auto resource = Allocate<Resource>(mResourceAllocator, name, desc);
        auto node = AllocatePOD<RenderGraphResourceNode>(mGraph, resource, 0);

        RGHandle handle;
        handle.Index = (uint16_t)mResources.size();
        handle.Node = (uint16_t)mResourceNodes.size();

        mResources.push_back(resource);
        mResourceNodes.push_back(node);

        return handle;
    }
}