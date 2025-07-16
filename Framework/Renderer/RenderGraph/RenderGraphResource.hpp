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
            : m_Name(name)
        {}
        virtual ~RenderGraphResource() {}

        virtual void Resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass);
        virtual void Realize() = 0;
        virtual RHI::RHIResource* GetResource() = 0;
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

        virtual RHI::RHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) = 0;
        virtual void Barrier(RHI::RHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) = 0;
    
    protected:
        eastl::string m_Name;

        DAGNodeID m_FirstPass = UINT32_MAX;
        DAGNodeID m_LastPass = 0;
        RHI::ERHIAccessFlags m_LastState = RHI::RHIAccessDiscard;

        bool m_bImported = false;
        bool m_bExported = false;
    };

    class RGTexture : public RenderGraphResource
    {
    public:
        using Desc = RHI::RHITextureDesc;

        RGTexture(RenderGraphResourceAllocator& allocator, const eastl::string& name, const Desc& desc);
        RGTexture(RenderGraphResourceAllocator& allocator, RHI::RHITexture* texture, RHI::ERHIAccessFlags state);
        ~RGTexture();

        RHI::RHITexture* GetTexture() { return m_pTexture; }
        RHI::RHIDescriptor* GetSRV();
        RHI::RHIDescriptor* GetUAV();
        RHI::RHIDescriptor* GetUAV(uint32_t mipLevel, uint32_t slice);

        virtual void Resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass) override;
        virtual void Realize() override;
        virtual RHI::RHIResource* GetResource() override { return m_pTexture; }
        virtual RHI::ERHIAccessFlags GetInitialState() override { return m_InitialState; }
        virtual RHI::RHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) override;
        virtual void Barrier(RHI::RHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) override;

    private:
        Desc m_Desc;
        RHI::RHITexture* m_pTexture = nullptr;
        RHI::ERHIAccessFlags m_InitialState = RHI::RHIAccessDiscard;
        RenderGraphResourceAllocator& m_Allocator;
    };

    class RGBuffer : public RenderGraphResource
    {
    public:
        using Desc = RHI::RHIBufferDesc;

        RGBuffer(RenderGraphResourceAllocator& allocator, const eastl::string& name, const Desc& desc);
        RGBuffer(RenderGraphResourceAllocator& allocator, RHI::RHIBuffer* buffer, RHI::ERHIAccessFlags state);
        ~RGBuffer();

        RHI::RHIBuffer* GetBuffer() { return m_pBuffer; }
        RHI::RHIDescriptor* GetSRV();
        RHI::RHIDescriptor* GetUAV();

        virtual void Resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass) override;
        virtual void Realize() override;
        virtual RHI::RHIResource* GetResource() override { return m_pBuffer; }
        virtual RHI::ERHIAccessFlags GetInitialState() override { return m_InitialState; }
        virtual RHI::RHIResource* GetAliasedPrevResource(RHI::ERHIAccessFlags& lastUsedState) override;
        virtual void Barrier(RHI::RHICommandList* pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter) override;

    private:
        Desc m_Desc;
        RHI::RHIBuffer* m_pBuffer = nullptr;
        RHI::ERHIAccessFlags m_InitialState = RHI::RHIAccessDiscard;
        RenderGraphResourceAllocator& m_Allocator;
    };
} // namespace RenderGraph
