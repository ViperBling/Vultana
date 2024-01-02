#include "TextureVK.hpp"
#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"
#include "TextureViewVK.hpp"
#include "GPUVK.hpp"
#include "QueueVK.hpp"
#include "CommandBufferVK.hpp"
#include "CommandListVK.hpp"
#include "SynchronousVK.hpp"

namespace RHI
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
        Destroy();
    }

    void TextureVK::Destroy()
    {
        if (mImage && mbOwnMemory)
        {
            vmaDestroyImage(mDevice.GetVmaAllocator(), mImage, mAllocation);
        }
        if (mImageView)
        {
            mDevice.GetVkDevice().destroyImageView(mImageView);
        }
    }

    RHITextureView *TextureVK::CreateTextureView(const TextureViewCreateInfo &createInfo)
    {
        return new TextureViewVK(*this, mDevice, createInfo);
    }

    void TextureVK::CreateImage(const TextureCreateInfo &createInfo)
    {
        GetAspect(createInfo);

        vk::ImageCreateInfo imageCI {};
        imageCI.setArrayLayers(mExtent.z);
        imageCI.setMipLevels(mMipLevels);
        imageCI.setExtent(GetVkExtent3D(createInfo.Extent));
        imageCI.setSamples(static_cast<vk::SampleCountFlagBits>(createInfo.Samples));
        imageCI.setImageType(VKEnumCast<RHITextureDimension, vk::ImageType>(createInfo.Dimension));
        imageCI.setFormat(VKEnumCast<RHIFormat, vk::Format>(createInfo.Format));
        imageCI.setUsage(GetVkImageUsageFlags(createInfo.Usage));

        VmaAllocationCreateInfo allocCI {};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;

        assert(vmaCreateImage(mDevice.GetVmaAllocator(), reinterpret_cast<VkImageCreateInfo*>(&imageCI), &allocCI, reinterpret_cast<VkImage*>(&mImage), &mAllocation, nullptr) == VK_SUCCESS);

        if (!createInfo.Name.empty())
        {
            mDevice.SetObjectName(vk::ObjectType::eImage, reinterpret_cast<uint64_t>(VkImage(mImage)), createInfo.Name.c_str());
        }
    }

    void TextureVK::GetAspect(const TextureCreateInfo &createInfo)
    {
        if (createInfo.Usage & RHITextureUsageBits::DepthStencilAttachment)
        {
            mAspect = vk::ImageAspectFlagBits::eDepth;
            if (createInfo.Format == RHIFormat::D32_FLOAT_S8_UINT || createInfo.Format == RHIFormat::D24_UNORM_S8_UINT)
            {
                mAspect |= vk::ImageAspectFlagBits::eStencil;
            }
        }
    }

    void TextureVK::TransitionToInitState(const TextureCreateInfo &createInfo)
    {
        if (createInfo.InitialState > RHITextureState::Undefined)
        {
            RHIQueue* queue = mDevice.GetQueue(RHICommandQueueType::Graphics, 0);
            assert(queue);

            auto fence = std::unique_ptr<RHIFence>(mDevice.CreateFence());
            auto commandBuffer = std::unique_ptr<RHICommandBuffer>(mDevice.CreateCommandBuffer());
            auto commandList = std::unique_ptr<RHICommandList>(commandBuffer->Begin());
            commandList->ResourceBarrier(RHIBarrier::Transition(this, RHITextureState::Undefined, createInfo.InitialState));
            commandList->End();

            queue->Submit(commandBuffer.get(), fence.get());
            fence->Wait();
        }
    }
}