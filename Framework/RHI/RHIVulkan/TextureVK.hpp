#pragma once

#include "RHI/RHITexture.hpp"
#include "Utilities/Math.hpp"

#include <memory>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class DeviceVK;

    class TextureVK : public RHITexture
    {
    public:
        NOCOPY(TextureVK)
        TextureVK(DeviceVK& device, const TextureCreateInfo& createInfo);
        TextureVK(DeviceVK& device, const TextureCreateInfo& createInfo, vk::Image image);
        ~TextureVK();
        void Destroy() override;
        RHITextureView* CreateTextureView(const TextureViewCreateInfo& createInfo) override;

        vk::Image GetImage() const { return mImage; }
        Vector3 GetExtent() const { return mExtent; }
        RHIFormat GetFormat() const { return mFormat; }
        vk::ImageSubresourceRange GetFullRange() { return {mAspect, 0, mMipLevels, 0, mExtent.z}; }
        
    private:
        void CreateImage(const TextureCreateInfo& createInfo);
        void GetAspect(const TextureCreateInfo& createInfo);
        void TransitionToInitState(const TextureCreateInfo& createInfo);

    private:
        friend class TextureViewVK;

        DeviceVK& mDevice;
        vk::Image mImage = VK_NULL_HANDLE;
        vk::ImageView mImageView = VK_NULL_HANDLE;
        vk::ImageAspectFlags mAspect = vk::ImageAspectFlagBits::eColor;
        VmaAllocation mAllocation = VK_NULL_HANDLE;

        Vector3 mExtent;
        RHIFormat mFormat;
        uint32_t mMipLevels = 0;
        uint8_t mSamples;
        bool mbOwnMemory;
    };
}