#pragma once

#include "DAG.hpp"
#include "RHI/RHI.hpp"

namespace RG
{
    class FRenderGraphEdge;
    class FRenderGraphPassBase;
    class FRenderGraphResourceAllocator;

    class FRenderGraphResource
    {
    public:
        FRenderGraphResource(const eastl::string& name)
            : m_Name(name)
        {}
        virtual ~FRenderGraphResource() {}

        virtual void Resolve(FRenderGraphEdge* edge, FRenderGraphPassBase* pass);
        virtual void Realize() = 0;
        virtual RHI::FRHIResource* GetResource() = 0;
        virtual RHI::ERHIAccessFlags GetInitialState() = 0;

        const eastl::string& GetName() const { return m_Name; }
        DAGNodeID GetFirstPassID() const { return m_FirstPass; }
        DAGNodeID GetLastPassID() const { return m_LastPass; }

        bool IsUsed() const { return m_FirstPass != UINT32_MAX; }
        bool IsImported() const { return m_bImported; }
        bool IsExported() const { return m_bExported; }
        bool SetExported(bool value) { return m_bExported = value; }

        RHI::ERHIAccessFlags GetFinalState() const { return m_LastState; }
        virtual void SetFinalState(RHI::ERHIAccessFlags state) { m_LastState = state; }

        bool IsOverlapping() const { return !IsImported() && !IsExported(); }

        virtual RHI::FRHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) = 0;
        virtual void Barrier(RHI::FRHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) = 0;
    
    protected:
        eastl::string m_Name;

        DAGNodeID m_FirstPass = UINT32_MAX;
        DAGNodeID m_LastPass = 0;
        RHI::ERHIAccessFlags m_LastState = RHI::RHIAccessDiscard;

        bool m_bImported = false;
        bool m_bExported = false;
    };

    class FRGTexture : public FRenderGraphResource
    {
    public:
        using Desc = RHI::FRHITextureDesc;

        FRGTexture(FRenderGraphResourceAllocator& allocator, const eastl::string& name, const Desc& desc);
        FRGTexture(FRenderGraphResourceAllocator& allocator, RHI::FRHITexture* texture, RHI::ERHIAccessFlags state);
        ~FRGTexture();

        RHI::FRHITexture* GetTexture() { return m_pTexture; }
        RHI::FRHIDescriptor* GetSRV();
        RHI::FRHIDescriptor* GetUAV();
        RHI::FRHIDescriptor* GetUAV(uint32_t mipLevel, uint32_t slice);

        virtual void Resolve(FRenderGraphEdge* edge, FRenderGraphPassBase* pass) override;
        virtual void Realize() override;
        virtual RHI::FRHIResource* GetResource() override { return m_pTexture; }
        virtual RHI::ERHIAccessFlags GetInitialState() override { return m_InitialState; }
        virtual RHI::FRHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) override;
        virtual void Barrier(RHI::FRHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) override;

    private:
        Desc m_Desc;
        RHI::FRHITexture* m_pTexture = nullptr;
        RHI::ERHIAccessFlags m_InitialState = RHI::RHIAccessDiscard;
        FRenderGraphResourceAllocator& m_Allocator;
    };

    class FRGBuffer : public FRenderGraphResource
    {
    public:
        using Desc = RHI::FRHIBufferDesc;

        FRGBuffer(FRenderGraphResourceAllocator& allocator, const eastl::string& name, const Desc& desc);
        FRGBuffer(FRenderGraphResourceAllocator& allocator, RHI::FRHIBuffer* buffer, RHI::ERHIAccessFlags state);
        ~FRGBuffer();

        RHI::FRHIBuffer* GetBuffer() { return m_pBuffer; }
        RHI::FRHIDescriptor* GetSRV();
        RHI::FRHIDescriptor* GetUAV();

        virtual void Resolve(FRenderGraphEdge* edge, FRenderGraphPassBase* pass) override;
        virtual void Realize() override;
        virtual RHI::FRHIResource* GetResource() override { return m_pBuffer; }
        virtual RHI::ERHIAccessFlags GetInitialState() override { return m_InitialState; }
        virtual RHI::FRHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) override;
        virtual void Barrier(RHI::FRHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) override;

    private:
        Desc m_Desc;
        RHI::FRHIBuffer* m_pBuffer = nullptr;
        RHI::ERHIAccessFlags m_InitialState = RHI::RHIAccessDiscard;
        FRenderGraphResourceAllocator& m_Allocator;
    };
} // namespace RenderGraph
