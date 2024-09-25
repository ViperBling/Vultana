#include "RHITextureVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIHeapVK.hpp"

#include "RHI/RHI.hpp"

#include "Utilities/Log.hpp"

namespace RHI
{
    RHITextureVK::RHITextureVK(RHIDeviceVK *device, const RHITextureDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
    }

    RHITextureVK::~RHITextureVK()
    {
        auto device = (RHIDeviceVK*)mpDevice;
        device->CancelDefaultLayoutTransition(this);

        if (!mbSwapchainImage)
        {
            device->Delete(mImage);
            device->Delete(mAllocation);
        }
        for (size_t i = 0; i < mRenderViews.size(); i++)
        {
            device->Delete(mRenderViews[i]);
        }
    }

    bool RHITextureVK::Create()
    {
        vk::Device deviceHandle = ((RHIDeviceVK*)mpDevice)->GetDevice();
        VmaAllocator allocator = ((RHIDeviceVK*)mpDevice)->GetVmaAllocator();

        vk::ImageCreateInfo imageCI = ToVulkanImageCreateInfo(mDesc);

        vk::Result res;

        if (mDesc.Heap != nullptr)
        {
            assert(mDesc.AllocationType == ERHIAllocationType::Placed);
            assert(mDesc.MemoryType == mDesc.Heap->GetDesc().MemoryType);

            res = (vk::Result)vmaCreateAliasingImage2(allocator, (VmaAllocation)mDesc.Heap->GetNativeHandle(), (VkDeviceSize)mDesc.HeapOffset, (VkImageCreateInfo*)&imageCI, (VkImage*)&mImage);
        }
        else
        {
            VmaAllocationCreateInfo allocCI = {};
            allocCI.usage = ToVmaUsage(mDesc.MemoryType);

            if (mDesc.AllocationType == ERHIAllocationType::Committed)
            {
                allocCI.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            }
            res = (vk::Result)vmaCreateImage(allocator, (VkImageCreateInfo*)&imageCI, &allocCI, (VkImage*)&mImage, &mAllocation, nullptr);
        }

        if (res != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHITextureVK] Failed to create image");
            return false;
        }
        auto dynamicLoder = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();
        SetDebugName(deviceHandle, vk::ObjectType::eImage, (uint64_t)(VkImage)mImage, mName.c_str(), dynamicLoder);

        if (mAllocation)
        {
            vmaSetAllocationName(allocator, mAllocation, mName.c_str());
        }
        ((RHIDeviceVK*)mpDevice)->EnqueueDefaultLayoutTransition(this);

        return true;
    }

    bool RHITextureVK::Create(vk::Image image)
    {
        auto device = (RHIDeviceVK*)mpDevice;

        mImage = image;
        mbSwapchainImage = true;

        auto dynamicLoder = device->GetDynamicLoader();
        SetDebugName(device->GetDevice(), vk::ObjectType::eImage, (uint64_t)(VkImage)mImage, mName.c_str(), dynamicLoder);
        
        device->EnqueueDefaultLayoutTransition(this);

        return true;
    }

    vk::ImageView RHITextureVK::GetRenderView(uint32_t mipSlice, uint32_t arraySlice)
    {
        assert(mDesc.Usage & (RHITextureUsageRenderTarget | RHITextureUsageDepthStencil));

        if (mRenderViews.empty())
        {
            mRenderViews.resize(mDesc.ArraySize * mDesc.MipLevels);
        }

        uint32_t index = mDesc.MipLevels * arraySlice + mipSlice;
        if (!mRenderViews[index])
        {
            vk::ImageViewCreateInfo imageViewCI {};
            imageViewCI.setImage(mImage);
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            imageViewCI.setFormat(ToVulkanFormat(mDesc.Format, true));
            imageViewCI.setSubresourceRange({ GetAspectFlags(mDesc.Format), mipSlice, 1, arraySlice, 1 });

            mRenderViews[index] = ((RHIDeviceVK*)mpDevice)->GetDevice().createImageView(imageViewCI);
        }

        return mRenderViews[index];
    }

    uint32_t RHITextureVK::GetRequiredStagingBufferSize() const
    {
        vk::MemoryRequirements memReq = ((RHIDeviceVK*)mpDevice)->GetDevice().getImageMemoryRequirements(mImage);
        return (uint32_t)memReq.size;
    }

    uint32_t RHITextureVK::GetRowPitch(uint32_t mipLevel) const
    {
        uint32_t minWidth = GetFormatBlockWidth(mDesc.Format);
        uint32_t width = std::max(minWidth, mDesc.Width >> mipLevel);

        return GetFormatRowPitch(mDesc.Format, width) * GetFormatBlockHeight(mDesc.Format);
    }

    void *RHITextureVK::GetSharedHandle() const
    {
        return nullptr;
    }
}