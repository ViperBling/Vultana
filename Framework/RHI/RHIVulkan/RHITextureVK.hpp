#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHITexture.hpp"

namespace RHI
{
    class FVulkanDevice;

    class FVulkanTexture : public FRHITexture
    {
    public:
        FVulkanTexture(FVulkanDevice* device, const FRHITextureDesc& desc, const eastl::string& name);
        ~FVulkanTexture();

        bool Create();
        bool Create(vk::Image image);
        bool IsSwapchainTexture() const { return m_bSwapchainImage; }
        vk::ImageView GetRenderView(uint32_t mipSlice, uint32_t arraySlice);

        virtual void* GetNativeHandle() const override { return m_Image; }

        virtual uint32_t GetRequiredStagingBufferSize() const override;
        virtual uint32_t GetRowPitch(uint32_t mipLevel = 0) const override;
        virtual void* GetSharedHandle() const override;

    private:
        vk::Image m_Image = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        bool m_bSwapchainImage = false;

        eastl::vector<vk::ImageView> m_RenderViews;
    };
}