#pragma once

#include "RenderGraphPass.hpp"
#include "RenderGraphHandle.hpp"
#include "RenderGraphResource.hpp"
#include "RenderGraphResourceAllocator.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/LinearAllocator.hpp"

#include <EASTL/unique_ptr.h>

namespace Renderer
{
    class FRendererBase;
}

namespace RG
{
    class FRenderGraphResourceNode;

    class FRenderGraph
    {
        friend class FRGBuilder;
    public:
        FRenderGraph(Renderer::FRendererBase* pRenderer);

        template<typename Data, typename Setup, typename Execute>
        TRenderGraphPass<Data>& AddPass(const eastl::string& name, RenderPassType type, const Setup& setup, const Execute& execute);

        void BeginEvent(const eastl::string& name) { m_EventNames.push_back(name); }
        void EndEvent();

        void Clear();
        void Compile();
        void Execute(Renderer::FRendererBase* pRenderer, RHI::FRHICommandList* pGraphicsCmdList, RHI::FRHICommandList* pComputeCmdList);

        void Present(const FRGHandle& handle, RHI::ERHIAccessFlags finalState);

        FRGHandle Import(RHI::FRHITexture* texture, RHI::ERHIAccessFlags state);
        FRGHandle Import(RHI::FRHIBuffer* buffer, RHI::ERHIAccessFlags state);

        FRGTexture* GetTexture(const FRGHandle& handle);
        FRGBuffer* GetBuffer(const FRGHandle& handle);

        const FDirectedAcyclicGraph& GetDAG() const { return m_Graph; }
        eastl::string Export();
    
    private:
        template<typename T, typename... ArgsT>
        T* Allocate(ArgsT&&... args);

        template<typename T, typename... ArgsT>
        T* AllocatePOD(ArgsT&&... args);

        template<typename Resource>
        FRGHandle Create(const typename Resource::Desc& desc, const eastl::string& name);

        FRGHandle Read(FRenderGraphPassBase* pass, const FRGHandle& input, RHI::ERHIAccessFlags usage, uint32_t subresource);
        FRGHandle Write(FRenderGraphPassBase* pass, const FRGHandle& input, RHI::ERHIAccessFlags usage, uint32_t subresource);

        FRGHandle WriteColor(FRenderGraphPassBase* pass, uint32_t colorIndex, const FRGHandle& input, uint32_t subresource, RHI::ERHIRenderPassLoadOp loadOp, const float4& clearColor);
        FRGHandle WriteDepth(FRenderGraphPassBase* pass, const FRGHandle& input, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil);
        FRGHandle ReadDepth(FRenderGraphPassBase* pass, const FRGHandle& input, uint32_t subresource);

    private:
        FLinearAllocator m_Allocator {512 * 1024};
        FRenderGraphResourceAllocator m_ResourceAllocator;
        FDirectedAcyclicGraph m_Graph;

        eastl::vector<eastl::string> m_EventNames;

        eastl::unique_ptr<RHI::FRHIFence> m_pGraphicsQueueFence;
        uint64_t m_GraphicsQueueFenceValue = 0;
        eastl::unique_ptr<RHI::FRHIFence> m_pComputeQueueFence;
        uint64_t m_ComputeQueueFenceValue = 0;

        eastl::vector<FRenderGraphPassBase*> m_Passes;
        eastl::vector<FRenderGraphResource*> m_Resources;
        eastl::vector<FRenderGraphResourceNode*> m_ResourceNodes;

        struct FObjFinalizer
        {
            void* Object;
            void(*Finalizer)(void*);
        };
        eastl::vector<FObjFinalizer> m_ObjFinalizers;

        struct FPresentTarget
        {
            FRenderGraphResource* Resource;
            RHI::ERHIAccessFlags State;
        };
        eastl::vector<FPresentTarget> m_OutputResources;
    };

    class FRenderGraphEvent
    {
    public:
        FRenderGraphEvent(FRenderGraph* pGraph, const eastl::string& name)
            : m_pGraph(pGraph)
        {
            m_pGraph->BeginEvent(name);
        }

        ~FRenderGraphEvent()
        {
            m_pGraph->EndEvent();
        }

    private:
        FRenderGraph* m_pGraph = nullptr;
    };
}

#define RENDER_GRAPH_EVENT(graph, name) RG::FRenderGraphEvent __graph_event__(graph, name)

#include "RenderGraph.inl"