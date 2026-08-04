#include "RHITextureVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIHeapVK.hpp"

#include "RHI/RHI.hpp"

#include "Utilities/Log.hpp"

namespace RHI
{
    FVulkanTexture::FVulkanTexture(FVulkanDevice *device, const FRHITextureDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
    }

    FVulkanTexture::~FVulkanTexture()
    {
        auto device = (FVulkanDevice*)m_pDevice;
        device->CancelDefaultLayoutTransition(this);

        if (!m_bSwapchainImage)
        {
            device->Delete(m_Image);
            device->Delete(m_Allocation);
        }
        for (size_t i = 0; i < m_RenderViews.size(); i++)
        {
            device->Delete(m_RenderViews[i]);
        }
    }

    bool FVulkanTexture::Create()
    {
        vk::Device deviceHandle = ((FVulkanDevice*)m_pDevice)->GetDevice();
        VmaAllocator allocator = ((FVulkanDevice*)m_pDevice)->GetVmaAllocator();

        vk::ImageCreateInfo imageCI = ToVulkanImageCreateInfo(m_Desc);

        vk::Result res;

        if (m_Desc.Heap != nullptr)
        {
            assert(m_Desc.AllocationType == ERHIAllocationType::Placed);
            assert(m_Desc.MemoryType == m_Desc.Heap->GetDesc().MemoryType);

            res = (vk::Result)vmaCreateAliasingImage2(allocator, (VmaAllocation)m_Desc.Heap->GetNativeHandle(), (VkDeviceSize)m_Desc.HeapOffset, (VkImageCreateInfo*)&imageCI, (VkImage*)&m_Image);
        }
        else
        {
            VmaAllocationCreateInfo allocCI = {};
            allocCI.usage = ToVmaUsage(m_Desc.MemoryType);

            if (m_Desc.AllocationType == ERHIAllocationType::Committed)
            {
                allocCI.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            }
            res = (vk::Result)vmaCreateImage(allocator, (VkImageCreateInfo*)&imageCI, &allocCI, (VkImage*)&m_Image, &m_Allocation, nullptr);
        }

        if (res != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHITextureVK] Failed to create image");
            return false;
        }
        auto dynamicLoder = ((FVulkanDevice*)m_pDevice)->GetDynamicLoader();
        SetDebugName(deviceHandle, vk::ObjectType::eImage, (uint64_t)(VkImage)m_Image, m_Name.c_str(), dynamicLoder);

        if (m_Allocation)
        {
            vmaSetAllocationName(allocator, m_Allocation, m_Name.c_str());
        }
        ((FVulkanDevice*)m_pDevice)->EnqueueDefaultLayoutTransition(this);

        return true;
    }

    bool FVulkanTexture::Create(vk::Image image)
    {
        auto device = (FVulkanDevice*)m_pDevice;

        m_Image = image;
        m_bSwapchainImage = true;

        auto dynamicLoder = device->GetDynamicLoader();
        SetDebugName(device->GetDevice(), vk::ObjectType::eImage, (uint64_t)(VkImage)m_Image, m_Name.c_str(), dynamicLoder);
        
        device->EnqueueDefaultLayoutTransition(this);

        return true;
    }

    vk::ImageView FVulkanTexture::GetRenderView(uint32_t mipSlice, uint32_t arraySlice)
    {
        assert(m_Desc.Usage & (RHITextureUsageRenderTarget | RHITextureUsageDepthStencil));

        if (m_RenderViews.empty())
        {
            m_RenderViews.resize(m_Desc.ArraySize * m_Desc.MipLevels);
        }

        uint32_t index = m_Desc.MipLevels * arraySlice + mipSlice;
        if (!m_RenderViews[index])
        {
            vk::ImageViewCreateInfo imageViewCI {};
            imageViewCI.setImage(m_Image);
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            imageViewCI.setFormat(ToVulkanFormat(m_Desc.Format, true));
            imageViewCI.setSubresourceRange({ GetAspectFlags(m_Desc.Format), mipSlice, 1, arraySlice, 1 });

            m_RenderViews[index] = ((FVulkanDevice*)m_pDevice)->GetDevice().createImageView(imageViewCI);
        }

        return m_RenderViews[index];
    }

    uint32_t FVulkanTexture::GetRequiredStagingBufferSize() const
    {
        vk::MemoryRequirements memReq = ((FVulkanDevice*)m_pDevice)->GetDevice().getImageMemoryRequirements(m_Image);
        return (uint32_t)memReq.size;
    }

    uint32_t FVulkanTexture::GetRowPitch(uint32_t mipLevel) const
    {
        uint32_t minWidth = GetFormatBlockWidth(m_Desc.Format);
        uint32_t width = eastl::max(minWidth, m_Desc.Width >> mipLevel);

        return GetFormatRowPitch(m_Desc.Format, width) * GetFormatBlockHeight(m_Desc.Format);
    }

    void *FVulkanTexture::GetSharedHandle() const
    {
        return nullptr;
    }
}