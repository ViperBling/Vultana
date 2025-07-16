#include "RHIHeapVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIHeapVK::RHIHeapVK(RHIDeviceVK *device, const RHIHeapDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
    }

    RHIHeapVK::~RHIHeapVK()
    {
        ((RHIDeviceVK*)m_pDevice)->Delete(m_Allocation);
    }

    bool RHIHeapVK::Create()
    {
        assert(m_Desc.Size % (64 * 1024) == 0);

        VmaAllocator allocator = ((RHIDeviceVK*)m_pDevice)->GetVmaAllocator();

        vk::MemoryRequirements memoryReq {};
        memoryReq.size = m_Desc.Size;
        memoryReq.alignment = 1;    // not used for dedicated allocations

        switch (m_Desc.MemoryType)
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
        vmaAI.usage = ToVmaUsage(m_Desc.MemoryType);

        VkResult result = vmaAllocateMemory(allocator, (VkMemoryRequirements*)&memoryReq, &vmaAI, &m_Allocation, nullptr);
        if (result != VK_SUCCESS)
        {
            VTNA_LOG_ERROR("[RHIHeapVK] Failed to create {}", m_Name);
            return false;
        }
        vmaSetAllocationName(allocator, m_Allocation, m_Name.c_str());

        return true;
    }
}