#include "RHIHeapVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIHeapVK::RHIHeapVK(RHIDeviceVK *device, const RHIHeapDesc &desc, const eastl::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
    }

    RHIHeapVK::~RHIHeapVK()
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mAllocation);
    }

    bool RHIHeapVK::Create()
    {
        assert(mDesc.Size % (64 * 1024) == 0);

        VmaAllocator allocator = ((RHIDeviceVK*)mpDevice)->GetVmaAllocator();

        vk::MemoryRequirements memoryReq {};
        memoryReq.size = mDesc.Size;
        memoryReq.alignment = 1;    // not used for dedicated allocations

        switch (mDesc.MemoryType)
        {
        case ERHIMemoryType::GPUOnly:
            memoryReq.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case ERHIMemoryType::CPUOnly:
            memoryReq.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case ERHIMemoryType::CPUToGPU:
            memoryReq.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            break;
        case ERHIMemoryType::GPUToCPU:
            memoryReq.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            break;
        default:
            break;
        }
        VmaAllocationCreateInfo vmaAI {};
        vmaAI.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        vmaAI.usage = ToVmaUsage(mDesc.MemoryType);

        VkResult result = vmaAllocateMemory(allocator, (VkMemoryRequirements*)&memoryReq, &vmaAI, &mAllocation, nullptr);
        if (result != VK_SUCCESS)
        {
            VTNA_LOG_ERROR("[RHIHeapVK] Failed to create {}", mName);
            return false;
        }
        vmaSetAllocationName(allocator, mAllocation, mName.c_str());

        return true;
    }
}