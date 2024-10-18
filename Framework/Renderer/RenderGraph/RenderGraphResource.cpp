#include "RenderGraphResource.hpp"
#include "RenderGraph.hpp"

namespace RG
{
    void RenderGraphResource::Resolve(RenderGraphEdge *edge, RenderGraphPassBase *pass)
    {
        if (pass->GetID() >= mLastPass)
        {
            mLastState = edge->GetUsage();
        }
        mFirstPass = std::min(mFirstPass, pass->GetID());
        mLastPass = std::max(mLastPass, pass->GetID());

        if (pass->GetType() == RenderPassType::AsyncCompute)
        {
            mFirstPass = std::min(mFirstPass, pass->GetWaitGraphicsPass());
            mLastPass = std::max(mLastPass, pass->GetSignalGraphicsPass());
        }
    }

    RGTexture::RGTexture(RenderGraphResourceAllocator &allocator, const std::string &name, const Desc &desc)
        : RenderGraphResource(name)
        , mAllocator(allocator)
    {
        mDesc = desc;
    }

    RGTexture::RGTexture(RenderGraphResourceAllocator &allocator, RHI::RHITexture *texture, RHI::ERHIAccessFlags state)
        : RenderGraphResource(texture->GetName())
        , mAllocator(allocator)
    {
        mDesc = texture->GetDesc();
        mpTexture = texture;
        mInitialState = state;
        mbImported = true;
    }

    RGTexture::~RGTexture()
    {
        if (!mbImported)
        {
            if (mbExported)
            {
                mAllocator.FreeNonOverlappingTexture(mpTexture, mLastState);
            }
            else
            {
                mAllocator.Free(mpTexture, mLastState, mbExported);
            }
        }
    }

    RHI::RHIDescriptor *RGTexture::GetSRV()
    {
        assert(!IsImported());

        RHI::RHIShaderResourceViewDesc desc;
        desc.Format = mpTexture->GetDesc().Format;

        return mAllocator.GetDescriptor(mpTexture, desc);
    }

    RHI::RHIDescriptor *RGTexture::GetUAV()
    {
        assert(!IsImported());

        RHI::RHIUnorderedAccessViewDesc desc;
        desc.Format = mpTexture->GetDesc().Format;

        return mAllocator.GetDescriptor(mpTexture, desc);
    }

    RHI::RHIDescriptor *RGTexture::GetUAV(uint32_t mipLevel, uint32_t slice)
    {
        assert(!IsImported());

        RHI::RHIUnorderedAccessViewDesc desc;
        desc.Format = mpTexture->GetDesc().Format;
        desc.Texture.MipSlice = mipLevel;
        desc.Texture.ArraySlice = slice;

        return mAllocator.GetDescriptor(mpTexture, desc);
    }

    void RGTexture::Resolve(RenderGraphEdge *edge, RenderGraphPassBase *pass)
    {
        RenderGraphResource::Resolve(edge, pass);

        RHI::ERHIAccessFlags usage = edge->GetUsage();
        if (usage & RHI::RHIAccessRTV)
        {
            mDesc.Usage |= RHI::RHITextureUsageRenderTarget;
        }
        if (usage & RHI::RHIAccessMaskUAV)
        {
            mDesc.Usage |= RHI::RHITextureUsageUnorderedAccess;
        }
        if (usage & (RHI::RHIAccessDSV | RHI::RHIAccessDSVReadOnly))
        {
            mDesc.Usage |= RHI::RHITextureUsageDepthStencil;
        }
    }

    void RGTexture::Realize()
    {
        if (!mbImported)
        {
            if (mbExported)
            {
                mpTexture = mAllocator.AllocateNonOverlappingTexture(mDesc, mName, mInitialState);
            }
            else
            {
                mpTexture = mAllocator.AllocateTexture(mFirstPass, mLastPass, mLastState, mDesc, mName, mInitialState);
            }
        }
    }

    RHI::RHIResource *RGTexture::GetAliasedPrevResource(RHI::ERHIAccessFlags &lastUsedState)
    {
        return mAllocator.GetAliasedPreviousResource(mpTexture, mFirstPass, lastUsedState);
    }

    void RGTexture::Barrier(RHI::RHICommandList *pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter)
    {
        pCmdList->TextureBarrier(mpTexture, subresource, accessBefore, accessAfter);
    }

    RGBuffer::RGBuffer(RenderGraphResourceAllocator &allocator, const std::string &name, const Desc &desc)
        : RenderGraphResource(name)
        , mAllocator(allocator)
    {
        mDesc = desc;
    }

    RGBuffer::RGBuffer(RenderGraphResourceAllocator &allocator, RHI::RHIBuffer *buffer, RHI::ERHIAccessFlags state)
        : RenderGraphResource(buffer->GetName())
        , mAllocator(allocator)
    {
        mDesc = buffer->GetDesc();
        mpBuffer = buffer;
        mInitialState = state;
        mbImported = true;
    }

    RGBuffer::~RGBuffer()
    {
        if (!mbImported)
        {
            mAllocator.Free(mpBuffer, mLastState, mbExported);
        }
    }

    RHI::RHIDescriptor *RGBuffer::GetSRV()
    {
        assert(!IsImported());

        const RHI::RHIBufferDesc& bufferDesc = mpBuffer->GetDesc();
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

        return mAllocator.GetDescriptor(mpBuffer, srvDesc);
    }

    RHI::RHIDescriptor *RGBuffer::GetUAV()
    {
        assert(!IsImported());

        const RHI::RHIBufferDesc& bufferDesc = mpBuffer->GetDesc();
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

        return mAllocator.GetDescriptor(mpBuffer, uavDesc);
    }

    void RGBuffer::Resolve(RenderGraphEdge *edge, RenderGraphPassBase *pass)
    {
        RenderGraphResource::Resolve(edge, pass);

        if (edge->GetUsage() & RHI::RHIAccessMaskUAV)
        {
            mDesc.Usage |= RHI::RHIBufferUsageUnorderedAccess;
        }
    }

    void RGBuffer::Realize()
    {
        if (!mbImported)
        {
            mpBuffer = mAllocator.AllocateBuffer(mFirstPass, mLastPass, mLastState, mDesc, mName, mInitialState);
        }
    }

    RHI::RHIResource *RGBuffer::GetAliasedPrevResource(RHI::ERHIAccessFlags &lastUsedState)
    {
        return mAllocator.GetAliasedPreviousResource(mpBuffer, mFirstPass, lastUsedState);
    }

    void RGBuffer::Barrier(RHI::RHICommandList *pCmdList, uint32_t subresource, RHI::ERHIAccessFlags accessBefore, RHI::ERHIAccessFlags accessAfter)
    {
        pCmdList->BufferBarrier(mpBuffer, accessBefore, accessAfter);
    }
}