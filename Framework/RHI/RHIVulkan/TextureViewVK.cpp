#include "TextureView.hpp"

#include "RHICommonVK.hpp"
#include "TextureVK.hpp"
#include "DeviceVK.hpp"

namespace Vultana
{
    TextureViewVK::TextureViewVK(TextureVK &texture, DeviceVK &device, const TextureViewCreateInfo &createInfo)
        : RHITextureView(createInfo)
        , mDevice(device)
        , mTexture(texture)
        , mBaseMipLevel(createInfo.BaseMipLevel)
        , mMipLevelCount(createInfo.MipLevelCount)
        , mBaseArrayLayer(createInfo.BaseArrayLayer)
        , mArrayLayerCount(createInfo.ArrayLayerCount)
    {
        CreateImageView(createInfo);
    }

    TextureViewVK::~TextureViewVK()
    {
        Destroy();
    }

    void TextureViewVK::Destroy()
    {
    }

    vk::ImageView TextureViewVK::GetVkImageView()
    {
        return mTexture.mImageView;
    }

    void TextureViewVK::CreateImageView(const TextureViewCreateInfo &createInfo)
    {
        if (!mTexture.mImageView)
        {
            vk::ImageViewCreateInfo imageViewCI {};
            imageViewCI.setFormat(VKEnumCast<RHIFormat, vk::Format>(mTexture.GetFormat()));
            imageViewCI.setImage(mTexture.GetImage());
            imageViewCI.setViewType(VKEnumCast<RHITextureViewDimension, vk::ImageViewType>(createInfo.Dimension));
            imageViewCI.setSubresourceRange({ GetVkAspectMask(createInfo.TextureType), mBaseMipLevel, mMipLevelCount, mBaseArrayLayer, mArrayLayerCount });
            mTexture.mImageView = mDevice.GetVkDevice().createImageView(imageViewCI);
        }
    }
}
