#include "TextureVK.hpp"
#include "RHICommonVK.hpp"

namespace Vultana
{
    TextureVK::TextureVK(DeviceVK &device, const TextureCreateInfo &createInfo)
        : RHITexture(createInfo)
        , mDevice(device)
        , mImage(VK_NULL_HANDLE)
        , mbOwnMemory(true)
        , mExtent(createInfo.Extent)
        , mFormat(createInfo.Format)
        , mMipLevels(createInfo.MipLevels)
        , mSamples(createInfo.Samples)
    {
        CreateImage(createInfo);
        TransitionToInitState(createInfo);
    }

    TextureVK::TextureVK(DeviceVK &device, const TextureCreateInfo &createInfo, vk::Image image)
        : RHITexture(createInfo)
        , mDevice(device)
        , mImage(image)
        , mbOwnMemory(false)
        , mExtent(createInfo.Extent)
        , mFormat(createInfo.Format)
        , mMipLevels(createInfo.MipLevels)
        , mSamples(createInfo.Samples)
    {
    }

    TextureVK::~TextureVK()
    {
    }

    void TextureVK::Destroy()
    {
    }

    RHITextureView *TextureVK::CreateTextureView(const TextureViewCreateInfo &createInfo)
    {
        return nullptr;
    }

    void TextureVK::CreateImage(const TextureCreateInfo &createInfo)
    {
    }

    void TextureVK::GetAspect(const TextureCreateInfo &createInfo)
    {
    }

    void TextureVK::TransitionToInitState(const TextureCreateInfo &createInfo)
    {
    }
}