#include "RenderGraphResourceAllocator.hpp"

namespace RG
{
    RenderGraphResourceAllocator::RenderGraphResourceAllocator(RHI::RHIDevice *device)
    {
    }
    
    RenderGraphResourceAllocator::~RenderGraphResourceAllocator()
    {
    }
    
    void RenderGraphResourceAllocator::Reset()
    {
    }

    RHI::RHITexture *RenderGraphResourceAllocator::AllocateNonOverlappingTexture(const RHI::RHITextureDesc &desc, const std::string &name, RHI::ERHIAccessFlags &initialState)
    {
        return nullptr;
    }

    void RenderGraphResourceAllocator::FreeNonOverlappingTexture(RHI::RHITexture *texture, RHI::ERHIAccessFlags state)
    {
    }

    RHI::RHITexture *RenderGraphResourceAllocator::AllocateTexture(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::RHITextureDesc &desc, const std::string &name, RHI::ERHIAccessFlags &initialState)
    {
        return nullptr;
    }

    RHI::RHIBuffer *RenderGraphResourceAllocator::AllocateBuffer(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::RHIBufferDesc &desc, const std::string &name, RHI::ERHIAccessFlags &initialState)
    {
        return nullptr;
    }

    void RenderGraphResourceAllocator::Free(RHI::RHIResource *resource, RHI::ERHIAccessFlags state, bool bIsSetState)
    {
    }

    RHI::RHIResource *RenderGraphResourceAllocator::GetAliasedPreviousResource(RHI::RHIResource *resource, uint32_t firstPass, RHI::ERHIAccessFlags &lastUsedState)
    {
        return nullptr;
    }

    RHI::RHIDescriptor *RenderGraphResourceAllocator::GetDescriptor(RHI::RHIResource *resource, const RHI::RHIShaderResourceViewDesc &desc)
    {
        return nullptr;
    }

    RHI::RHIDescriptor *RenderGraphResourceAllocator::GetDescriptor(RHI::RHIResource *resource, const RHI::RHIUnorderedAccessViewDesc &desc)
    {
        return nullptr;
    }

    void RenderGraphResourceAllocator::CheckHeapUsage(Heap &heap)
    {
    }

    void RenderGraphResourceAllocator::DeleteDescriptor(RHI::RHIResource *resource)
    {
    }

    void RenderGraphResourceAllocator::AllocateHeap(uint32_t size)
    {
    }
}