#include "RenderGraphResourceAllocator.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

#include <cassert>

namespace RG
{
    FRenderGraphResourceAllocator::FRenderGraphResourceAllocator(RHI::FRHIDevice *device)
    {
        m_pDevice = device;
    }
    
    FRenderGraphResourceAllocator::~FRenderGraphResourceAllocator()
    {
        for (auto iter = m_AllocatedHeaps.begin(); iter != m_AllocatedHeaps.end(); ++iter)
        {
            const FHeap& heap = *iter;
            for (size_t i = 0; i < heap.Resources.size(); i++)
            {
                DeleteDescriptor(heap.Resources[i].Resource);
                delete heap.Resources[i].Resource;
            }
            delete heap.Heap;
        }

        for (auto iter = m_FreeOverlappingTextures.begin(); iter != m_FreeOverlappingTextures.end(); ++iter)
        {
            DeleteDescriptor(iter->Texture);
            delete iter->Texture;
        }
    }
    
    void FRenderGraphResourceAllocator::Reset()
    {
        for (auto iter = m_AllocatedHeaps.begin(); iter != m_AllocatedHeaps.end();)
        {
            FHeap& heap = *iter;
            CheckHeapUsage(heap);
            if (heap.Resources.empty())
            {
                delete heap.Heap;
                iter = m_AllocatedHeaps.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        uint64_t currentFrame = m_pDevice->GetFrameID();
        for (auto iter = m_FreeOverlappingTextures.begin(); iter != m_FreeOverlappingTextures.end();)
        {
            if (currentFrame - iter->LastUsedFrame > 30)
            {
                DeleteDescriptor(iter->Texture);
                delete iter->Texture;
                iter = m_FreeOverlappingTextures.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    RHI::FRHITexture *FRenderGraphResourceAllocator::AllocateNonOverlappingTexture(const RHI::FRHITextureDesc& desc, const eastl::string& name, RHI::ERHIAccessFlags& initialState)
    {
        for (auto iter = m_FreeOverlappingTextures.begin(); iter != m_FreeOverlappingTextures.end(); ++iter)
        {
            RHI::FRHITexture* texture = iter->Texture;
            if (texture->GetDesc() == desc)
            {
                initialState = iter->LastUsedState;
                m_FreeOverlappingTextures.erase(iter);
                return texture;
            }
        }
        if (RHI::IsDepthFormat(desc.Format))
        {
            initialState = RHI::RHIAccessDSV;
        }
        else if (desc.Usage & RHI::RHITextureUsageRenderTarget)
        {
            initialState = RHI::RHIAccessRTV;
        }
        else if (desc.Usage & RHI::RHITextureUsageUnorderedAccess)
        {
            initialState = RHI::RHIAccessMaskUAV;
        }
        return m_pDevice->CreateTexture(desc, "RGTexture_" + name);
    }

    void FRenderGraphResourceAllocator::FreeNonOverlappingTexture(RHI::FRHITexture *texture, RHI::ERHIAccessFlags state)
    {
        if (texture != nullptr)
        {
            m_FreeOverlappingTextures.push_back({texture, state, m_pDevice->GetFrameID()});
        }
    }

    RHI::FRHITexture *FRenderGraphResourceAllocator::AllocateTexture(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::FRHITextureDesc &desc, const eastl::string &name, RHI::ERHIAccessFlags &initialState)
    {
        FLifeTimeRange lifeTime = {firstPass, lastPass};
        uint32_t textureSize = m_pDevice->GetAllocationSize(desc);
        for (size_t i = 0; i < m_AllocatedHeaps.size(); i++)
        {
            FHeap& heap = m_AllocatedHeaps[i];
            if (heap.Heap->GetDesc().Size < textureSize || heap.IsOverlapping(lifeTime)) continue;

            for (size_t j = 0; j < heap.Resources.size(); j++)
            {
                FAliasedResource& aliasedRes = heap.Resources[j];
                if (aliasedRes.Resource->IsTexture() && !aliasedRes.LifeTime.IsUsed() && ((RHI::FRHITexture*)aliasedRes.Resource)->GetDesc() == desc)
                {
                    aliasedRes.LifeTime = lifeTime;
                    initialState = aliasedRes.LastUsedState;
                    aliasedRes.LastUsedState = lastState;
                    return (RHI::FRHITexture*)aliasedRes.Resource;
                }
            }
            RHI::FRHITextureDesc newDesc = desc;
            newDesc.Heap = heap.Heap;

            FAliasedResource aliasedTexture;
            aliasedTexture.Resource = m_pDevice->CreateTexture(newDesc, "RGTexture_" + name);
            aliasedTexture.LifeTime = lifeTime;
            aliasedTexture.LastUsedState = lastState;
            heap.Resources.push_back(aliasedTexture);
            
            if (RHI::IsDepthFormat(desc.Format))
            {
                initialState = RHI::RHIAccessDSV;
            }
            else if (desc.Usage & RHI::RHITextureUsageRenderTarget)
            {
                initialState = RHI::RHIAccessRTV;
            }
            else if (desc.Usage & RHI::RHITextureUsageUnorderedAccess)
            {
                initialState = RHI::RHIAccessMaskUAV;
            }
            assert(aliasedTexture.Resource != nullptr);
            return (RHI::FRHITexture*)aliasedTexture.Resource;
        }
        AllocateHeap(textureSize);
        return AllocateTexture(firstPass, lastPass, lastState, desc, name, initialState);
    }

    RHI::FRHIBuffer *FRenderGraphResourceAllocator::AllocateBuffer(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::FRHIBufferDesc &desc, const eastl::string &name, RHI::ERHIAccessFlags &initialState)
    {
        FLifeTimeRange lifeTime = {firstPass, lastPass};
        uint32_t bufferSize = desc.Size;

        for (size_t i = 0; i < m_AllocatedHeaps.size(); i++)
        {
            FHeap& heap = m_AllocatedHeaps[i];
            if (heap.Heap->GetDesc().Size < bufferSize || heap.IsOverlapping(lifeTime)) continue;

            for (size_t j = 0; j < heap.Resources.size(); j++)
            {
                FAliasedResource& aliasedRes = heap.Resources[j];
                if (aliasedRes.Resource->IsBuffer() && !aliasedRes.LifeTime.IsUsed() && ((RHI::FRHIBuffer*)aliasedRes.Resource)->GetDesc() == desc)
                {
                    aliasedRes.LifeTime = lifeTime;
                    initialState = aliasedRes.LastUsedState;
                    aliasedRes.LastUsedState = lastState;
                    return (RHI::FRHIBuffer*)aliasedRes.Resource;
                }
            }
            RHI::FRHIBufferDesc newDesc = desc;
            newDesc.Heap = heap.Heap;

            FAliasedResource aliasedBuffer;
            aliasedBuffer.Resource = m_pDevice->CreateBuffer(newDesc, "RGBuffer_" + name);
            aliasedBuffer.LifeTime = lifeTime;
            aliasedBuffer.LastUsedState = lastState;
            heap.Resources.push_back(aliasedBuffer);

            initialState = RHI::RHIAccessDiscard;
            assert(aliasedBuffer.Resource != nullptr);
            return (RHI::FRHIBuffer*)aliasedBuffer.Resource;
        }
        AllocateHeap(bufferSize);
        return AllocateBuffer(firstPass, lastPass, lastState, desc, name, initialState);
    }

    void FRenderGraphResourceAllocator::Free(RHI::FRHIResource *resource, RHI::ERHIAccessFlags state, bool bIsSetState)
    {
        if (resource != nullptr)
        {
            for (size_t i = 0; i < m_AllocatedHeaps.size(); i++)
            {
                FHeap& heap = m_AllocatedHeaps[i];
                for (size_t j = 0; j < heap.Resources.size(); j++)
                {
                    FAliasedResource& aliasedRes = heap.Resources[j];
                    if (aliasedRes.Resource == resource)
                    {
                        aliasedRes.LifeTime.Reset();
                        aliasedRes.LastUsedFrame = m_pDevice->GetFrameID();
                        if (bIsSetState)
                        {
                            aliasedRes.LastUsedState = state;
                        }
                        return;
                    }
                }
            }
            assert(false);
        }
    }

    RHI::FRHIResource *FRenderGraphResourceAllocator::GetAliasedPreviousResource(RHI::FRHIResource *resource, uint32_t firstPass, RHI::ERHIAccessFlags &lastUsedState)
    {
        for (size_t i = 0; i < m_AllocatedHeaps.size(); i++)
        {
            FHeap& heap = m_AllocatedHeaps[i];
            if (!heap.Contains(resource)) continue;

            FAliasedResource* aliasedRes = nullptr;
            RHI::FRHIResource* prevResource = nullptr;
            uint32_t prevResourceLastPass = 0;

            for (size_t j = 0; j < heap.Resources.size(); j++)
            {
                FAliasedResource& res = heap.Resources[j];
                if (res.Resource != resource && res.LifeTime.LastPass < firstPass && res.LifeTime.LastPass > prevResourceLastPass)
                {
                    aliasedRes = &res;
                    prevResource = res.Resource;
                    lastUsedState = res.LastUsedState;
                    prevResourceLastPass = res.LifeTime.LastPass;
                }
            }
            if (aliasedRes != nullptr)
            {
                aliasedRes->LastUsedState |= RHI::RHIAccessDiscard;
            }
            return prevResource;
        }
        assert(false);
        return nullptr;
    }

    RHI::FRHIDescriptor *FRenderGraphResourceAllocator::GetDescriptor(RHI::FRHIResource *resource, const RHI::FRHIShaderResourceViewDesc &desc)
    {
        for (size_t i = 0; i < m_AllocatedSRVs.size(); i++)
        {
            if (m_AllocatedSRVs[i].Resource == resource && m_AllocatedSRVs[i].Desc == desc)
            {
                return m_AllocatedSRVs[i].Descriptor;
            }
        }
        RHI::FRHIDescriptor* srv = m_pDevice->CreateShaderResourceView(resource, desc, resource->GetName());
        m_AllocatedSRVs.push_back({resource, srv, desc});
        return srv;
    }

    RHI::FRHIDescriptor *FRenderGraphResourceAllocator::GetDescriptor(RHI::FRHIResource *resource, const RHI::FRHIUnorderedAccessViewDesc &desc)
    {
        for (size_t i = 0; i < m_AllocatedUAVs.size(); i++)
        {
            if (m_AllocatedUAVs[i].Resource == resource && m_AllocatedUAVs[i].Desc == desc)
            {
                return m_AllocatedUAVs[i].Descriptor;
            }
        }
        RHI::FRHIDescriptor* uav = m_pDevice->CreateUnorderedAccessView(resource, desc, resource->GetName());
        m_AllocatedUAVs.push_back({resource, uav, desc});
        return uav;
    }

    void FRenderGraphResourceAllocator::CheckHeapUsage(FHeap &heap)
    {
        uint64_t currentFrame = m_pDevice->GetFrameID();
        for (auto iter = heap.Resources.begin(); iter != heap.Resources.end();)
        {
            const FAliasedResource aliasedRes = *iter;
            if (currentFrame - aliasedRes.LastUsedFrame > 30)
            {
                DeleteDescriptor(aliasedRes.Resource);
                delete aliasedRes.Resource;
                iter = heap.Resources.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    void FRenderGraphResourceAllocator::DeleteDescriptor(RHI::FRHIResource *resource)
    {
        for (auto iter = m_AllocatedSRVs.begin(); iter != m_AllocatedSRVs.end();)
        {
            if (iter->Resource == resource)
            {
                delete iter->Descriptor;
                iter = m_AllocatedSRVs.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        for (auto iter = m_AllocatedUAVs.begin(); iter != m_AllocatedUAVs.end();)
        {
            if (iter->Resource == resource)
            {
                delete iter->Descriptor;
                iter = m_AllocatedUAVs.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    void FRenderGraphResourceAllocator::AllocateHeap(uint32_t size)
    {
        RHI::FRHIHeapDesc heapDesc;
        heapDesc.Size = RoundUpPow2(size, 64u * 1024);

        eastl::string heapName = fmt::format("RG Heap {:.1} MB", heapDesc.Size / (1024.0f * 1024.0f)).c_str();

        FHeap heap;
        heap.Heap = m_pDevice->CreateHeap(heapDesc, heapName);
        m_AllocatedHeaps.push_back(heap);
    }
}