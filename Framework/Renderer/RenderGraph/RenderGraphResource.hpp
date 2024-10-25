#pragma once

#include "DAG.hpp"
#include "RHI/RHI.hpp"

namespace RG
{
    class RenderGraphEdge;
    class RenderGraphPassBase;
    class RenderGraphResourceAllocator;

    class RenderGraphResource
    {
    public:
        RenderGraphResource(const eastl::string& name)
            : mName(name)
        {}
        virtual ~RenderGraphResource() {}

        virtual void Resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass);
        virtual void Realize() = 0;
        virtual RHI::RHIResource* GetResource() = 0;
        virtual RHI::ERHIAccessFlags GetInitialState() = 0;

        const eastl::string& GetName() const { return mName; }
        DAGNodeID GetFirstPassID() const { return mFirstPass; }
        DAGNodeID GetLastPassID() const { return mLastPass; }

        bool IsUsed() const { return mFirstPass != UINT32_MAX; }
        bool IsImported() const { return mbImported; }
        bool IsExported() const { return mbExported; }
        bool SetExported(bool value) { return mbExported = value; }

        RHI::ERHIAccessFlags GetFinalState() const { return mLastState; }
        virtual void SetFinalState(RHI::ERHIAccessFlags state) { mLastState = state; }

        bool IsOverlapping() const { return !IsImported() && !IsExported(); }

        virtual RHI::RHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) = 0;
        virtual void Barrier(RHI::RHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) = 0;
    
    protected:
        eastl::string mName;

        DAGNodeID mFirstPass = UINT32_MAX;
        DAGNodeID mLastPass = 0;
        RHI::ERHIAccessFlags mLastState = RHI::RHIAccessDiscard;

        bool mbImported = false;
        bool mbExported = false;
    };

    class RGTexture : public RenderGraphResource
    {
    public:
        using Desc = RHI::RHITextureDesc;

        RGTexture(RenderGraphResourceAllocator& allocator, const eastl::string& name, const Desc& desc);
        RGTexture(RenderGraphResourceAllocator& allocator, RHI::RHITexture* texture, RHI::ERHIAccessFlags state);
        ~RGTexture();

        RHI::RHITexture* GetTexture() { return mpTexture; }
        RHI::RHIDescriptor* GetSRV();
        RHI::RHIDescriptor* GetUAV();
        RHI::RHIDescriptor* GetUAV(uint32_t mipLevel, uint32_t slice);

        virtual void Resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass) override;
        virtual void Realize() override;
        virtual RHI::RHIResource* GetResource() override { return mpTexture; }
        virtual RHI::ERHIAccessFlags GetInitialState() override { return mInitialState; }
        virtual RHI::RHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) override;
        virtual void Barrier(RHI::RHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) override;

    private:
        Desc mDesc;
        RHI::RHITexture* mpTexture = nullptr;
        RHI::ERHIAccessFlags mInitialState = RHI::RHIAccessDiscard;
        RenderGraphResourceAllocator& mAllocator;
    };

    class RGBuffer : public RenderGraphResource
    {
    public:
        using Desc = RHI::RHIBufferDesc;

        RGBuffer(RenderGraphResourceAllocator& allocator, const eastl::string& name, const Desc& desc);
        RGBuffer(RenderGraphResourceAllocator& allocator, RHI::RHIBuffer* buffer, RHI::ERHIAccessFlags state);
        ~RGBuffer();

        RHI::RHIBuffer* GetBuffer() { return mpBuffer; }
        RHI::RHIDescriptor* GetSRV();
        RHI::RHIDescriptor* GetUAV();

        virtual void Resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass) override;
        virtual void Realize() override;
        virtual RHI::RHIResource* GetResource() override { return mpBuffer; }
        virtual RHI::ERHIAccessFlags GetInitialState() override { return mInitialState; }
        virtual RHI::RHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) override;
        virtual void Barrier(RHI::RHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) override;

    private:
        Desc mDesc;
        RHI::RHIBuffer* mpBuffer = nullptr;
        RHI::ERHIAccessFlags mInitialState = RHI::RHIAccessDiscard;
        RenderGraphResourceAllocator& mAllocator;
    };
} // namespace RenderGraph
