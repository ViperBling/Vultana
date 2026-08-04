#include "RHIBufferVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIHeapVK.hpp"

#include "Utilities/Utility.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    FVulkanBuffer::FVulkanBuffer(FVulkanDevice *device, const FRHIBufferDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
    }

    FVulkanBuffer::~FVulkanBuffer()
    {
        ((FVulkanDevice*)m_pDevice)->Delete(m_Buffer);
        ((FVulkanDevice*)m_pDevice)->Delete(m_Allocation);
    }

    bool FVulkanBuffer::Create()
    {
        vk::Device device = ((FVulkanDevice*)m_pDevice)->GetDevice();
        auto dynamicLoader = ((FVulkanDevice*)m_pDevice)->GetDynamicLoader();
        VmaAllocator allocator = ((FVulkanDevice*)m_pDevice)->GetVmaAllocator();

        vk::BufferCreateInfo bufferCI {};
        bufferCI.size = m_Desc.Size;
        bufferCI.sharingMode = vk::SharingMode::eExclusive;
        bufferCI.usage = 
            vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eTransferSrc |
            vk::BufferUsageFlagBits::eIndexBuffer |
            // vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eIndirectBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress;

        if (m_Desc.Usage & RHIBufferUsageConstantBuffer)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
        }
        if (m_Desc.Usage & (RHIBufferUsageStructuredBuffer | RHIBufferUsageRawBuffer ))
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        }
        if (m_Desc.Usage & RHIBufferUsageTypedBuffer)
        {
            // if (m_Desc.Usage & RHIBufferUsageUnorderedAccess)
            // {
            //     bufferCI.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
            // }
            // else
            // {
            //     bufferCI.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
            // }
            bufferCI.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
        }
        if (m_Desc.Usage & RHIBufferUsageUnorderedAccess)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
        }

            vk::Result result;
        if (m_Desc.Heap != nullptr)
        {
            assert(m_Desc.AllocationType == ERHIAllocationType::Placed);
            assert(m_Desc.MemoryType == m_Desc.Heap->GetDesc().MemoryType);
            assert(m_Desc.Size + m_Desc.HeapOffset <= m_Desc.Heap->GetDesc().Size);

            result = (vk::Result)vmaCreateAliasingBuffer2(allocator, (VmaAllocation)m_Desc.Heap->GetNativeHandle(), (vk::DeviceSize)m_Desc.HeapOffset, (VkBufferCreateInfo*)&bufferCI, (VkBuffer*)(&m_Buffer));
        }
        else
        {
            VmaAllocationCreateInfo vmaAllocCI {};
            vmaAllocCI.usage = ToVmaUsage(m_Desc.MemoryType);
            if (m_Desc.AllocationType == ERHIAllocationType::Committed)
            {
                vmaAllocCI.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            }
            if (m_Desc.MemoryType != ERHIMemoryType::GPUOnly)
            {
                vmaAllocCI.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            }
            VmaAllocationInfo vmaAllocInfo {};
            result = (vk::Result)vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bufferCI, &vmaAllocCI, (VkBuffer*)(&m_Buffer), &m_Allocation, &vmaAllocInfo);

            m_pData = vmaAllocInfo.pMappedData;
        }
        
        if (result != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHIBufferVK] Failed to create buffer: {}", m_Name);
            return false;
        }

        SetDebugName(device, vk::ObjectType::eBuffer, m_Buffer, m_Name.c_str(), dynamicLoader);

        if (m_Allocation)
        {
            vmaSetAllocationName(allocator, m_Allocation, m_Name.c_str());
        }
        return true;
    }

    void * FVulkanBuffer::GetCPUAddress()
    {
        return m_pData;
    }
    
    uint64_t FVulkanBuffer::GetGPUAddress()
    {
        vk::BufferDeviceAddressInfo addressInfo {};
        addressInfo.buffer = m_Buffer;

        return ((FVulkanDevice*)m_pDevice)->GetDevice().getBufferAddress(addressInfo);
    }

    uint32_t FVulkanBuffer::GetRequiredStagingBufferSize() const
    {
        vk::MemoryRequirements memReq {};
        ((FVulkanDevice*)m_pDevice)->GetDevice().getBufferMemoryRequirements(m_Buffer, &memReq);
        return (uint32_t)memReq.size;
    }
}