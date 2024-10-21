#include "RenderGraphResourceAllocator.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

#include <cassert>

namespace RG
{
    RenderGraphResourceAllocator::RenderGraphResourceAllocator(RHI::RHIDevice *device)
    {
        mpDevice = device;
    }
    
    RenderGraphResourceAllocator::~RenderGraphResourceAllocator()
    {
        for (auto iter = mAllocatedHeaps.begin(); iter != mAllocatedHeaps.end(); ++iter)
        {
            const FHeap& heap = *iter;
            for (size_t i = 0; i < heap.Resources.size(); i++)
            {
                DeleteDescriptor(heap.Resources[i].Resource);
                delete heap.Resources[i].Resource;
            }
            delete heap.Heap;
        }

        for (auto iter = mFreeOverlappingTextures.begin(); iter != mFreeOverlappingTextures.end(); ++iter)
        {
            DeleteDescriptor(iter->Texture);
            delete iter->Texture;
        }
    }
    
    void RenderGraphResourceAllocator::Reset()
    {
        for (auto iter = mAllocatedHeaps.begin(); iter != mAllocatedHeaps.end();)
        {
            FHeap& heap = *iter;
            CheckHeapUsage(heap);
            if (heap.Resources.empty())
            {
                delete heap.Heap;
                iter = mAllocatedHeaps.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        uint64_t currentFrame = mpDevice->GetFrameID();
        for (auto iter = mFreeOverlappingTextures.begin(); iter != mFreeOverlappingTextures.end();)
        {
            if (currentFrame - iter->LastUsedFrame > 30)
            {
                DeleteDescriptor(iter->Texture);
                delete iter->Texture;
                iter = mFreeOverlappingTextures.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    RHI::RHITexture *RenderGraphResourceAllocator::AllocateNonOverlappingTexture(const RHI::RHITextureDesc& desc, const std::string& name, RHI::ERHIAccessFlags& initialState)
    {
        for (auto iter = mFreeOverlappingTextures.begin(); iter != mFreeOverlappingTextures.end(); ++iter)
        {
            RHI::RHITexture* texture = iter->Texture;
            if (texture->GetDesc() == desc)
            {
                initialState = iter->LastUsedState;
                mFreeOverlappingTextures.erase(iter);
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
        return mpDevice->CreateTexture(desc, "RGTexture_" + name);
    }

    void RenderGraphResourceAllocator::FreeNonOverlappingTexture(RHI::RHITexture *texture, RHI::ERHIAccessFlags state)
    {
        if (texture != nullptr)
        {
            mFreeOverlappingTextures.push_back({texture, state, mpDevice->GetFrameID()});
        }
    }

    RHI::RHITexture *RenderGraphResourceAllocator::AllocateTexture(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::RHITextureDesc &desc, const std::string &name, RHI::ERHIAccessFlags &initialState)
    {
        LifeTimeRange lifeTime = {firstPass, lastPass};
        uint32_t textureSize = mpDevice->GetAllocationSize(desc);
        for (size_t i = 0; i < mAllocatedHeaps.size(); i++)
        {
            FHeap& heap = mAllocatedHeaps[i];
            if (heap.Heap->GetDesc().Size < textureSize || heap.IsOverlapping(lifeTime)) continue;

            for (size_t j = 0; j < heap.Resources.size(); j++)
            {
                AliasedResource& aliasedRes = heap.Resources[j];
                if (aliasedRes.Resource->IsTexture() && !aliasedRes.LifeTime.IsUsed() && ((RHI::RHITexture*)aliasedRes.Resource)->GetDesc() == desc)
                {
                    aliasedRes.LifeTime = lifeTime;
                    initialState = aliasedRes.LastUsedState;
                    aliasedRes.LastUsedState = lastState;
                    return (RHI::RHITexture*)aliasedRes.Resource;
                }
            }
            RHI::RHITextureDesc newDesc = desc;
            newDesc.Heap = heap.Heap;

            AliasedResource aliasedTexture;
            aliasedTexture.Resource = mpDevice->CreateTexture(newDesc, "RGTexture_" + name);
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
            return (RHI::RHITexture*)aliasedTexture.Resource;
        }
        AllocateHeap(textureSize);
        return AllocateTexture(firstPass, lastPass, lastState, desc, name, initialState);
    }

    RHI::RHIBuffer *RenderGraphResourceAllocator::AllocateBuffer(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::RHIBufferDesc &desc, const std::string &name, RHI::ERHIAccessFlags &initialState)
    {
        LifeTimeRange lifeTime = {firstPass, lastPass};
        uint32_t bufferSize = desc.Size;

        for (size_t i = 0; i < mAllocatedHeaps.size(); i++)
        {
            FHeap& heap = mAllocatedHeaps[i];
            if (heap.Heap->GetDesc().Size < bufferSize || heap.IsOverlapping(lifeTime)) continue;

            for (size_t j = 0; j < heap.Resources.size(); j++)
            {
                AliasedResource& aliasedRes = heap.Resources[j];
                if (aliasedRes.Resource->IsBuffer() && !aliasedRes.LifeTime.IsUsed() && ((RHI::RHIBuffer*)aliasedRes.Resource)->GetDesc() == desc)
                {
                    aliasedRes.LifeTime = lifeTime;
                    initialState = aliasedRes.LastUsedState;
                    aliasedRes.LastUsedState = lastState;
                    return (RHI::RHIBuffer*)aliasedRes.Resource;
                }
            }
            RHI::RHIBufferDesc newDesc = desc;
            newDesc.Heap = heap.Heap;

            AliasedResource aliasedBuffer;
            aliasedBuffer.Resource = mpDevice->CreateBuffer(newDesc, "RGBuffer_" + name);
            aliasedBuffer.LifeTime = lifeTime;
            aliasedBuffer.LastUsedState = lastState;
            heap.Resources.push_back(aliasedBuffer);

            initialState = RHI::RHIAccessDiscard;
            assert(aliasedBuffer.Resource != nullptr);
            return (RHI::RHIBuffer*)aliasedBuffer.Resource;
        }
        AllocateHeap(bufferSize);
        return AllocateBuffer(firstPass, lastPass, lastState, desc, name, initialState);
    }

    void RenderGraphResourceAllocator::Free(RHI::RHIResource *resource, RHI::ERHIAccessFlags state, bool bIsSetState)
    {
        if (resource != nullptr)
        {
            for (size_t i = 0; i < mAllocatedHeaps.size(); i++)
            {
                FHeap& heap = mAllocatedHeaps[i];
                for (size_t j = 0; j < heap.Resources.size(); j++)
                {
                    AliasedResource& aliasedRes = heap.Resources[j];
                    if (aliasedRes.Resource == resource)
                    {
                        aliasedRes.LifeTime.Reset();
                        aliasedRes.LastUsedFrame = mpDevice->GetFrameID();
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

    RHI::RHIResource *RenderGraphResourceAllocator::GetAliasedPreviousResource(RHI::RHIResource *resource, uint32_t firstPass, RHI::ERHIAccessFlags &lastUsedState)
    {
        for (size_t i = 0; i < mAllocatedHeaps.size(); i++)
        {
            FHeap& heap = mAllocatedHeaps[i];
            if (!heap.Contains(resource)) continue;

            AliasedResource* aliasedRes = nullptr;
            RHI::RHIResource* prevResource = nullptr;
            uint32_t prevResourceLastPass = 0;

            for (size_t j = 0; j < heap.Resources.size(); j++)
            {
                AliasedResource& res = heap.Resources[j];
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

    RHI::RHIDescriptor *RenderGraphResourceAllocator::GetDescriptor(RHI::RHIResource *resource, const RHI::RHIShaderResourceViewDesc &desc)
    {
        for (size_t i = 0; i < mAllocatedSRVs.size(); i++)
        {
            if (mAllocatedSRVs[i].Resource == resource && mAllocatedSRVs[i].Desc == desc)
            {
                return mAllocatedSRVs[i].Descriptor;
            }
        }
        RHI::RHIDescriptor* srv = mpDevice->CreateShaderResourceView(resource, desc, resource->GetName());
        mAllocatedSRVs.push_back({resource, srv, desc});
        return srv;
    }

    RHI::RHIDescriptor *RenderGraphResourceAllocator::GetDescriptor(RHI::RHIResource *resource, const RHI::RHIUnorderedAccessViewDesc &desc)
    {
        for (size_t i = 0; i < mAllocatedUAVs.size(); i++)
        {
            if (mAllocatedUAVs[i].Resource == resource && mAllocatedUAVs[i].Desc == desc)
            {
                return mAllocatedUAVs[i].Descriptor;
            }
        }
        RHI::RHIDescriptor* uav = mpDevice->CreateUnorderedAccessView(resource, desc, resource->GetName());
        mAllocatedUAVs.push_back({resource, uav, desc});
        return uav;
    }

    void RenderGraphResourceAllocator::CheckHeapUsage(FHeap &heap)
    {
        uint64_t currentFrame = mpDevice->GetFrameID();
        for (auto iter = heap.Resources.begin(); iter != heap.Resources.end();)
        {
            const AliasedResource aliasedRes = *iter;
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

    void RenderGraphResourceAllocator::DeleteDescriptor(RHI::RHIResource *resource)
    {
        for (auto iter = mAllocatedSRVs.begin(); iter != mAllocatedSRVs.end();)
        {
            if (iter->Resource == resource)
            {
                delete iter->Descriptor;
                iter = mAllocatedSRVs.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        for (auto iter = mAllocatedUAVs.begin(); iter != mAllocatedUAVs.end();)
        {
            if (iter->Resource == resource)
            {
                delete iter->Descriptor;
                iter = mAllocatedUAVs.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    void RenderGraphResourceAllocator::AllocateHeap(uint32_t size)
    {
        RHI::RHIHeapDesc heapDesc;
        heapDesc.Size = RoundUpPow2(size, 64u * 1024);

        std::string heapName = fmt::format("RG Heap {:.1} MB", heapDesc.Size / (1024.0f * 1024.0f)).c_str();

        FHeap heap;
        heap.Heap = mpDevice->CreateHeap(heapDesc, heapName);
        mAllocatedHeaps.push_back(heap);
    }
}