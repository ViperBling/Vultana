#pragma once

#include "RenderGraphPass.hpp"
#include "RenderGraphHandle.hpp"
#include "RenderGraphResource.hpp"
#include "RenderGraphResourceAllocator.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/LinearAllocator.hpp"

#include <memory>

namespace Renderer
{
    class RendererBase;
}

namespace RG
{
    class RenderGraphResourceNode;

    class RenderGraph
    {
        friend class RGBuilder;
    public:
        RenderGraph(Renderer::RendererBase* pRenderer);

        template<typename Data, typename Setup, typename Execute>
        RenderGraphPass<Data>& AddPass(const std::string& name, RenderPassType type, const Setup& setup, const Execute& execute);

        void BeginEvent(const std::string& name) { mEventNames.push_back(name); }
        void EndEvent();

        void Clear();
        void Compile();
        void Execute(Renderer::RendererBase* pRenderer, RHI::RHICommandList* pGraphicsCmdList, RHI::RHICommandList* pComputeCmdList);

        void Present(const RGHandle& handle, RHI::ERHIAccessFlags finalState);

        RGHandle Import(RHI::RHITexture* texture, RHI::ERHIAccessFlags state);
        RGHandle Import(RHI::RHIBuffer* buffer, RHI::ERHIAccessFlags state);

        RGTexture* GetTexture(const RGHandle& handle);
        RGBuffer* GetBuffer(const RGHandle& handle);

        const DirectedAcyclicGraph& GetDAG() const { return mGraph; }
        std::string Export() {}
    
    private:
        template<typename T, typename... ArgsT>
        T* Allocate(ArgsT&&... args);

        template<typename T, typename... ArgsT>
        T* AllocatePOD(ArgsT&&... args);

        template<typename Resource>
        RGHandle Create(const typename Resource::Desc& desc, const std::string& name);

        RGHandle Read(RenderGraphPassBase* pass, const RGHandle& input, RHI::ERHIAccessFlags usage, uint32_t subresource);
        RGHandle Write(RenderGraphPassBase* pass, const RGHandle& input, RHI::ERHIAccessFlags usage, uint32_t subresource);

        RGHandle WriteColor(RenderGraphPassBase* pass, uint32_t colorIndex, const RGHandle& input, uint32_t subresource, RHI::ERHIRenderPassLoadOp loadOp, const float4& clearColor);
        RGHandle WriteDepth(RenderGraphPassBase* pass, const RGHandle& input, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil);
        RGHandle ReadDepth(RenderGraphPassBase* pass, const RGHandle& input, uint32_t subresource);

    private:
        LinearAllocator mAllocator {512 * 1024};
        RenderGraphResourceAllocator mResourceAllocator;
        DirectedAcyclicGraph mGraph;

        std::vector<std::string> mEventNames;

        std::unique_ptr<RHI::RHIFence> mpGraphicsQueueFence;
        uint64_t mGraphicsQueueFenceValue = 0;
        std::unique_ptr<RHI::RHIFence> mpComputeQueueFence;
        uint64_t mComputeQueueFenceValue = 0;

        std::vector<RenderGraphPassBase*> mPasses;
        std::vector<RenderGraphResource*> mResources;
        std::vector<RenderGraphResourceNode*> mResourceNodes;

        struct ObjFinalizer
        {
            void* Object;
            void(*Finalizer)(void*);
        };
        std::vector<ObjFinalizer> mObjFinalizers;

        struct PresentTarget
        {
            RenderGraphResource* Resource;
            RHI::ERHIAccessFlags State;
        };
        std::vector<PresentTarget> mOutputResources;
    };

    class RenderGraphEvent
    {
    public:
        RenderGraphEvent(RenderGraph* pGraph, const std::string& name)
            : mpGraph(pGraph)
        {
            mpGraph->BeginEvent(name);
        }

        ~RenderGraphEvent()
        {
            mpGraph->EndEvent();
        }

    private:
        RenderGraph* mpGraph = nullptr;
    };
}

#define RENDER_GRAPH_EVENT(graph, name) RG::RenderGraphEvent __graph_event__(graph, name)

#include "RenderGraph.inl"