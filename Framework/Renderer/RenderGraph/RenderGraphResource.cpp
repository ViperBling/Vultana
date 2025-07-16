#include "RenderGraphResource.hpp"
#include "RenderGraph.hpp"

namespace RG
{
    void RenderGraphResource::Resolve(RenderGraphEdge *edge, RenderGraphPassBase *pass)
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

    RGTexture::RGTexture(RenderGraphResourceAllocator &allocator, const eastl::string &name, const Desc &desc)
        : RenderGraphResource(name)
        , m_Allocator(allocator)
    {
        m_Desc = desc;
    }

    RGTexture::RGTexture(RenderGraphResourceAllocator &allocator, RHI::RHITexture *texture, RHI::ERHIAccessFlags state)
        : RenderGraphResource(texture->GetName())
        , m_Allocator(allocator)
    {
        m_Desc = texture->GetDesc();
        m_pTexture = texture;
        m_InitialState = state;
        m_bImported = true;
    }

    RGTexture::~RGTexture()
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

    RHI::RHIDescriptor *RGTexture::GetSRV()
    {
        assert(!IsImported());

        RHI::RHIShaderResourceViewDesc desc;
        desc.Format = m_pTexture->GetDesc().Format;

        return m_Allocator.GetDescriptor(m_pTexture, desc);
    }

    RHI::RHIDescriptor *RGTexture::GetUAV()
    {
        assert(!IsImported());

        RHI::RHIUnorderedAccessViewDesc desc;
        desc.Format = m_pTexture->GetDesc().Format;

        return m_Allocator.GetDescriptor(m_pTexture, desc);
    }

    RHI::RHIDescriptor *RGTexture::GetUAV(uint32_t mipLevel, uint32_t slice)
    {
        assert(!IsImported());

        RHI::RHIUnorderedAccessViewDesc desc;
        desc.Format = m_pTexture->GetDesc().Format;
        desc.Texture.MipSlice = mipLevel;
        desc.Texture.ArraySlice = slice;

        return m_Allocator.GetDescriptor(m_pTexture, desc);
    }

    void RGTexture::Resolve(RenderGraphEdge *edge, RenderGraphPassBase *pass)
    {
        RenderGraphResource::Resolve(edge, pass);

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

    void RGTexture::Realize()
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

    RHI::RHIResource *RGTexture::GetAliasedPrevResource(RHI::ERHIAccessFlags &lastUsedState)
    {
        return m_Allocator.GetAliasedPreviousResource(m_pTexture, m_FirstPass, lastUsedState);
    }

    void RGTexture::Barrier(RHI::RHICommandList *pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter)
    {
        pCmdList->TextureBarrier(m_pTexture, subresource, accessBefore, accessAfter);
    }

    RGBuffer::RGBuffer(RenderGraphResourceAllocator &allocator, const eastl::string &name, const Desc &desc)
        : RenderGraphResource(name)
        , m_Allocator(allocator)
    {
        m_Desc = desc;
    }

    RGBuffer::RGBuffer(RenderGraphResourceAllocator &allocator, RHI::RHIBuffer *buffer, RHI::ERHIAccessFlags state)
        : RenderGraphResource(buffer->GetName())
        , m_Allocator(allocator)
    {
        m_Desc = buffer->GetDesc();
        m_pBuffer = buffer;
        m_InitialState = state;
        m_bImported = true;
    }

    RGBuffer::~RGBuffer()
    {
        if (!m_bImported)
        {
            m_Allocator.Free(m_pBuffer, m_LastState, m_bExported);
        }
    }

    RHI::RHIDescriptor *RGBuffer::GetSRV()
    {
        assert(!IsImported());

        const RHI::RHIBufferDesc& bufferDesc = m_pBuffer->GetDesc();
        RHI::RHIShaderResourceViewDesc srvDesc;
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

    RHI::RHIDescriptor *RGBuffer::GetUAV()
    {
        assert(!IsImported());

        const RHI::RHIBufferDesc& bufferDesc = m_pBuffer->GetDesc();
        assert(bufferDesc.Usage & RHI::RHIBufferUsageUnorderedAccess);

        RHI::RHIUnorderedAccessViewDesc uavDesc;
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

    void RGBuffer::Resolve(RenderGraphEdge *edge, RenderGraphPassBase *pass)
    {
        RenderGraphResource::Resolve(edge, pass);

        if (edge->GetUsage() & RHI::RHIAccessMaskUAV)
        {
            m_Desc.Usage |= RHI::RHIBufferUsageUnorderedAccess;
        }
    }

    void RGBuffer::Realize()
    {
        if (!m_bImported)
        {
            m_pBuffer = m_Allocator.AllocateBuffer(m_FirstPass, m_LastPass, m_LastState, m_Desc, m_Name, m_InitialState);
        }
    }

    RHI::RHIResource *RGBuffer::GetAliasedPrevResource(RHI::ERHIAccessFlags &lastUsedState)
    {
        return m_Allocator.GetAliasedPreviousResource(m_pBuffer, m_FirstPass, lastUsedState);
    }

    void RGBuffer::Barrier(RHI::RHICommandList *pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter)
    {
        pCmdList->BufferBarrier(m_pBuffer, accessBefore, accessAfter);
    }
}