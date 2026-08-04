#pragma once

#include "RHI/RHI.hpp"

namespace RG
{
    class FRenderGraphResourceAllocator
    {
        struct FLifeTimeRange
        {
            uint32_t FirstPass = UINT32_MAX;
            uint32_t LastPass = 0;

            void Reset() { FirstPass = UINT32_MAX; LastPass = 0; }
            bool IsUsed() const { return FirstPass != UINT32_MAX; }
            bool IsOverlapping(const FLifeTimeRange& other) const
            {
                if (IsUsed()) return FirstPass <= other.LastPass && LastPass >= other.FirstPass;
                else return false;
            }
        };

        struct FAliasedResource
        {
            RHI::FRHIResource* Resource = nullptr;
            FLifeTimeRange LifeTime;
            uint64_t LastUsedFrame = 0;
            RHI::ERHIAccessFlags LastUsedState = RHI::RHIAccessDiscard;
        };

        struct FHeap
        {
            RHI::FRHIHeap* Heap = nullptr;
            eastl::vector<FAliasedResource> Resources;

            bool IsOverlapping(const FLifeTimeRange& lifeTime) const
            {
                for (const FAliasedResource& resource : Resources)
                {
                    if (resource.LifeTime.IsOverlapping(lifeTime)) return true;
                }
                return false;
            }

            bool Contains(RHI::FRHIResource* resource) const
            {
                for (const FAliasedResource& aliasedResource : Resources)
                {
                    if (aliasedResource.Resource == resource) return true;
                }
                return false;
            }
        };

        struct FSRVDescriptor
        {
            RHI::FRHIResource* Resource;
            RHI::FRHIDescriptor* Descriptor;
            RHI::FRHIShaderResourceViewDesc Desc;
        };

        struct FUAVDescriptor
        {
            RHI::FRHIResource* Resource;
            RHI::FRHIDescriptor* Descriptor;
            RHI::FRHIUnorderedAccessViewDesc Desc;
        };

    public:
        FRenderGraphResourceAllocator(RHI::FRHIDevice* device);
        ~FRenderGraphResourceAllocator();

        void Reset();

        RHI::FRHITexture* AllocateNonOverlappingTexture(const RHI::FRHITextureDesc& desc, const eastl::string& name, RHI::ERHIAccessFlags& initialState);
        void FreeNonOverlappingTexture(RHI::FRHITexture* texture, RHI::ERHIAccessFlags state);

        RHI::FRHITexture* AllocateTexture(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::FRHITextureDesc& desc, const eastl::string& name, RHI::ERHIAccessFlags& initialState);
        RHI::FRHIBuffer* AllocateBuffer(uint32_t firstPass, uint32_t lastPass, RHI::ERHIAccessFlags lastState, const RHI::FRHIBufferDesc& desc, const eastl::string& name, RHI::ERHIAccessFlags& initialState);
        void Free(RHI::FRHIResource* resource, RHI::ERHIAccessFlags state, bool bIsSetState);

        RHI::FRHIResource* GetAliasedPreviousResource(RHI::FRHIResource* resource, uint32_t firstPass, RHI::ERHIAccessFlags& lastUsedState);

        RHI::FRHIDescriptor* GetDescriptor(RHI::FRHIResource* resource, const RHI::FRHIShaderResourceViewDesc& desc);
        RHI::FRHIDescriptor* GetDescriptor(RHI::FRHIResource* resource, const RHI::FRHIUnorderedAccessViewDesc& desc);

    private:
        void CheckHeapUsage(FHeap& heap);
        void DeleteDescriptor(RHI::FRHIResource* resource);
        void AllocateHeap(uint32_t size);

    private:
        RHI::FRHIDevice* m_pDevice = nullptr;

        eastl::vector<FHeap> m_AllocatedHeaps;

        struct FNonOverlappingTexture
        {
            RHI::FRHITexture* Texture;
            RHI::ERHIAccessFlags LastUsedState;
            uint64_t LastUsedFrame;
        };
        eastl::vector<FNonOverlappingTexture> m_FreeOverlappingTextures;

        eastl::vector<FSRVDescriptor> m_AllocatedSRVs;
        eastl::vector<FUAVDescriptor> m_AllocatedUAVs;
    };
} // namespace RG
