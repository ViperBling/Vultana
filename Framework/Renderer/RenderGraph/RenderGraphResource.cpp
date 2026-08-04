#include "RenderGraphResource.hpp"
#include "RenderGraph.hpp"

namespace RG
{
    void FRenderGraphResource::Resolve(FRenderGraphEdge *edge, FRenderGraphPassBase *pass)
    {
        if (pass->GetID() >= m_LastPass)
        {
            m_LastState = edge->GetUsage();
        }
        m_FirstPass = eastl::min(m_FirstPass, pass->GetID());
        m_LastPass = eastl::max(m_LastPass, pass->GetID());

        if (pass->GetType() == RenderPassType::AsyncCompute)
        {
            m_FirstPass = eastl::min(m_FirstPass, pass->GetWaitGraphicsPass());
            m_LastPass = eastl::max(m_LastPass, pass->GetSignalGraphicsPass());
        }
    }

    FRGTexture::FRGTexture(FRenderGraphResourceAllocator &allocator, const eastl::string &name, const Desc &desc)
        : FRenderGraphResource(name)
        , m_Allocator(allocator)
    {
        m_Desc = desc;
    }

    FRGTexture::FRGTexture(FRenderGraphResourceAllocator &allocator, RHI::FRHITexture *texture, RHI::ERHIAccessFlags state)
        : FRenderGraphResource(texture->GetName())
        , m_Allocator(allocator)
    {
        m_Desc = texture->GetDesc();
        m_pTexture = texture;
        m_InitialState = state;
        m_bImported = true;
    }

    FRGTexture::~FRGTexture()
    {
        if (!m_bImported)
        {
            if (m_bExported)
            {
                m_Allocator.FreeNonOverlappingTexture(m_pTexture, m_LastState);
            }
            else
            {
                m_Allocator.Free(m_pTexture, m_LastState, m_bExported);
            }
        }
    }

    RHI::FRHIDescriptor *FRGTexture::GetSRV()
    {
        assert(!IsImported());

        RHI::FRHIShaderResourceViewDesc desc;
        desc.Format = m_pTexture->GetDesc().Format;

        return m_Allocator.GetDescriptor(m_pTexture, desc);
    }

    RHI::FRHIDescriptor *FRGTexture::GetUAV()
    {
        assert(!IsImported());

        RHI::FRHIUnorderedAccessViewDesc desc;
        desc.Format = m_pTexture->GetDesc().Format;

        return m_Allocator.GetDescriptor(m_pTexture, desc);
    }

    RHI::FRHIDescriptor *FRGTexture::GetUAV(uint32_t mipLevel, uint32_t slice)
    {
        assert(!IsImported());

        RHI::FRHIUnorderedAccessViewDesc desc;
        desc.Format = m_pTexture->GetDesc().Format;
        desc.Texture.MipSlice = mipLevel;
        desc.Texture.ArraySlice = slice;

        return m_Allocator.GetDescriptor(m_pTexture, desc);
    }

    void FRGTexture::Resolve(FRenderGraphEdge *edge, FRenderGraphPassBase *pass)
    {
        FRenderGraphResource::Resolve(edge, pass);

        RHI::ERHIAccessFlags usage = edge->GetUsage();
        if (usage & RHI::RHIAccessRTV)
        {
            m_Desc.Usage |= RHI::RHITextureUsageRenderTarget;
        }
        if (usage & RHI::RHIAccessMaskUAV)
        {
            m_Desc.Usage |= RHI::RHITextureUsageUnorderedAccess;
        }
        if (usage & (RHI::RHIAccessDSV | RHI::RHIAccessDSVReadOnly))
        {
            m_Desc.Usage |= RHI::RHITextureUsageDepthStencil;
        }
    }

    void FRGTexture::Realize()
    {
        if (!m_bImported)
        {
            if (m_bExported)
            {
                m_pTexture = m_Allocator.AllocateNonOverlappingTexture(m_Desc, m_Name, m_InitialState);
            }
            else
            {
                m_pTexture = m_Allocator.AllocateTexture(m_FirstPass, m_LastPass, m_LastState, m_Desc, m_Name, m_InitialState);
            }
        }
    }

    RHI::FRHIResource *FRGTexture::GetAliasedPrevResource(RHI::ERHIAccessFlags &lastUsedState)
    {
        return m_Allocator.GetAliasedPreviousResource(m_pTexture, m_FirstPass, lastUsedState);
    }

    void FRGTexture::Barrier(RHI::FRHICommandList *pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter)
    {
        pCmdList->TextureBarrier(m_pTexture, subresource, accessBefore, accessAfter);
    }

    FRGBuffer::FRGBuffer(FRenderGraphResourceAllocator &allocator, const eastl::string &name, const Desc &desc)
        : FRenderGraphResource(name)
        , m_Allocator(allocator)
    {
        m_Desc = desc;
    }

    FRGBuffer::FRGBuffer(FRenderGraphResourceAllocator &allocator, RHI::FRHIBuffer *buffer, RHI::ERHIAccessFlags state)
        : FRenderGraphResource(buffer->GetName())
        , m_Allocator(allocator)
    {
        m_Desc = buffer->GetDesc();
        m_pBuffer = buffer;
        m_InitialState = state;
        m_bImported = true;
    }

    FRGBuffer::~FRGBuffer()
    {
        if (!m_bImported)
        {
            m_Allocator.Free(m_pBuffer, m_LastState, m_bExported);
        }
    }

    RHI::FRHIDescriptor *FRGBuffer::GetSRV()
    {
        assert(!IsImported());

        const RHI::FRHIBufferDesc& bufferDesc = m_pBuffer->GetDesc();
        RHI::FRHIShaderResourceViewDesc srvDesc;
        srvDesc.Format = bufferDesc.Format;

        if (bufferDesc.Usage & RHI::RHIBufferUsageStructuredBuffer)
        {
            srvDesc.Type = RHI::ERHIShaderResourceViewType::StructuredBuffer;
        }
        else if (bufferDesc.Usage & RHI::RHIBufferUsageTypedBuffer)
        {
            srvDesc.Type = RHI::ERHIShaderResourceViewType::TypedBuffer;
        }
        else if (bufferDesc.Usage & RHI::RHIBufferUsageRawBuffer)
        {
            srvDesc.Type = RHI::ERHIShaderResourceViewType::RawBuffer;
        }
        srvDesc.Buffer.Offset = 0;
        srvDesc.Buffer.Size = bufferDesc.Size;

        return m_Allocator.GetDescriptor(m_pBuffer, srvDesc);
    }

    RHI::FRHIDescriptor *FRGBuffer::GetUAV()
    {
        assert(!IsImported());

        const RHI::FRHIBufferDesc& bufferDesc = m_pBuffer->GetDesc();
        assert(bufferDesc.Usage & RHI::RHIBufferUsageUnorderedAccess);

        RHI::FRHIUnorderedAccessViewDesc uavDesc;
        uavDesc.Format = bufferDesc.Format;

        if (bufferDesc.Usage & RHI::RHIBufferUsageStructuredBuffer)
        {
            uavDesc.Type = RHI::ERHIUnorderedAccessViewType::StructuredBuffer;
        }
        else if (bufferDesc.Usage & RHI::RHIBufferUsageTypedBuffer)
        {
            uavDesc.Type = RHI::ERHIUnorderedAccessViewType::TypedBuffer;
        }
        else if (bufferDesc.Usage & RHI::RHIBufferUsageRawBuffer)
        {
            uavDesc.Type = RHI::ERHIUnorderedAccessViewType::RawBuffer;
        }
        uavDesc.Buffer.Offset = 0;
        uavDesc.Buffer.Size = bufferDesc.Size;

        return m_Allocator.GetDescriptor(m_pBuffer, uavDesc);
    }

    void FRGBuffer::Resolve(FRenderGraphEdge *edge, FRenderGraphPassBase *pass)
    {
        FRenderGraphResource::Resolve(edge, pass);

        if (edge->GetUsage() & RHI::RHIAccessMaskUAV)
        {
            m_Desc.Usage |= RHI::RHIBufferUsageUnorderedAccess;
        }
    }

    void FRGBuffer::Realize()
    {
        if (!m_bImported)
        {
            m_pBuffer = m_Allocator.AllocateBuffer(m_FirstPass, m_LastPass, m_LastState, m_Desc, m_Name, m_InitialState);
        }
    }

    RHI::FRHIResource *FRGBuffer::GetAliasedPrevResource(RHI::ERHIAccessFlags &lastUsedState)
    {
        return m_Allocator.GetAliasedPreviousResource(m_pBuffer, m_FirstPass, lastUsedState);
    }

    void FRGBuffer::Barrier(RHI::FRHICommandList *pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter)
    {
        pCmdList->BufferBarrier(m_pBuffer, accessBefore, accessAfter);
    }
}