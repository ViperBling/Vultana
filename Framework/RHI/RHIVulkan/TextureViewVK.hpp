#pragma once

#include "RHI/RHITextureView.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    class TextureVK;
    class DeviceVK;

    class TextureViewVK : public RHITextureView
    {
    public:
        NOCOPY(TextureViewVK);
        TextureViewVK(TextureVK& texture, DeviceVK& device, const TextureViewCreateInfo& createInfo);
        ~TextureViewVK();
        void Destroy() override;

        vk::ImageView GetVkImageView();
        TextureVK& GetTexture() const { return mTexture; }
        uint8_t GetArrayLayerCount() const { return mArrayLayerCount; }

    private:
        void CreateImageView(const TextureViewCreateInfo& createInfo);

    private:
        DeviceVK& mDevice;
        TextureVK& mTexture;
        uint8_t mBaseMipLevel;
        uint8_t mMipLevelCount;
        uint8_t mBaseArrayLayer;
        uint8_t mArrayLayerCount;
    };
}