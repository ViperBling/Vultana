#pragma once

#include "RHI/RHI.hpp"

namespace RG
{
    class RenderGraphResourceAllocator
    {
        struct LifeTimeRange
        {
            uint32_t FirstPass = UINT32_MAX;
            uint32_t LastPass = 0;

            void Reset() { FirstPass = UINT32_MAX; LastPass = 0; }
            bool IsUsed() const { return FirstPass != UINT32_MAX; }
            bool IsOverlapping(const LifeTimeRange& other) const
            {
                if (IsUsed()) return FirstPass <= other.LastPass && LastPass >= other.FirstPass;
                else return false;
            }
        };

        struct AliasedResource
        {
            RHI::RHIResource* Resource = nullptr;
            LifeTimeRange LifeTime;
            uint64_t LastUsedFrame = 0;
            RHI::ERHIAccessFlags LastUsedState = RHI::RHIAccessDiscard;
        };

        struct FHeap
        {
            RHI::RHIHeap* Heap = nullptr;
            std::vector<AliasedResource> Resources;

            bool IsOverlapping(const LifeTimeRange& lifeTime) const
            {
                for (const AliasedResource& resource : Resources)
                {
                    if (resource.LifeTime.IsOverlapping(lifeTime)) return true;
                }
                return false;
            }

            bool Contains(RHI::RHIResource* resource) const
            {
                for (const AliasedResource& aliasedResource : Resources)
                {
                    if (aliasedResource.Resource == resource) return true;
                }
                return false;
            }
        };

        struct SRVDescriptor
        {
            RHI::RHIResource* Resource;
            RHI::RHIDescriptor* Descriptor;
            RHI::RHIShaderResourceViewDesc Desc;
        };

        struct UAVDescriptor
        {
            RHI::RHIResource* Resource;
            RHI::RHIDescriptor* Descriptor;
            RHI::RHIUnorderedAccessViewDesc Desc;
        };

    public:
        RenderGraphResourceAllocator(RHI::RHIDevice* device);
        ~RenderGraphResourceAllocator();

        void Reset();

        RHI::RHITexture* AllocateNonOverlappingTexture(const RHI::RHITextureDesc& desc, const std::string& name, RHI::ERHIAccessFlags& initialState);
        void FreeNonOverlappingTexture(RHI::RHITexture* texture, RHI::ERHIAccessFlags state);

        RHI::RHITexture* AllocateTexture(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::RHITextureDesc& desc, const std::string& name, RHI::ERHIAccessFlags& initialState);
        RHI::RHIBuffer* AllocateBuffer(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::RHIBufferDesc& desc, const std::string& name, RHI::ERHIAccessFlags& initialState);
        void Free(RHI::RHIResource* resource, RHI::ERHIAccessFlags state, bool bIsSetState);

        RHI::RHIResource* GetAliasedPreviousResource(RHI::RHIResource* resource, uint32_t firstPass, RHI::ERHIAccessFlags& lastUsedState);

        RHI::RHIDescriptor* GetDescriptor(RHI::RHIResource* resource, const RHI::RHIShaderResourceViewDesc& desc);
        RHI::RHIDescriptor* GetDescriptor(RHI::RHIResource* resource, const RHI::RHIUnorderedAccessViewDesc& desc);

    private:
        void CheckHeapUsage(FHeap& heap);
        void DeleteDescriptor(RHI::RHIResource* resource);
        void AllocateHeap(uint32_t size);

    private:
        RHI::RHIDevice* mpDevice = nullptr;

        std::vector<FHeap> mAllocatedHeaps;

        struct NonOverlappingTexture
        {
            RHI::RHITexture* Texture;
            RHI::ERHIAccessFlags LastUsedState;
            uint64_t LastUsedFrame;
        };
        std::vector<NonOverlappingTexture> mFreeOverlappingTextures;

        std::vector<SRVDescriptor> mAllocatedSRVs;
        std::vector<UAVDescriptor> mAllocatedUAVs;
    };
} // namespace RG
