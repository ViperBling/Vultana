#include "RHIBufferVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIHeapVK.hpp"

#include "Utilities/Utility.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIBufferVK::RHIBufferVK(RHIDeviceVK *device, const RHIBufferDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
    }

    RHIBufferVK::~RHIBufferVK()
    {
        ((RHIDeviceVK*)m_pDevice)->Delete(m_Buffer);
        ((RHIDeviceVK*)m_pDevice)->Delete(m_Allocation);
    }

    bool RHIBufferVK::Create()
    {
        vk::Device device = ((RHIDeviceVK*)m_pDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)m_pDevice)->GetDynamicLoader();
        VmaAllocator allocator = ((RHIDeviceVK*)m_pDevice)->GetVmaAllocator();

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
            if (m_Desc.Usage & RHIBufferUsageUnorderedAccess)
            {
                bufferCI.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
            }
            else
            {
                bufferCI.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
            }
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

    void * RHIBufferVK::GetCPUAddress()
    {
        return m_pData;
    }
    
    uint64_t RHIBufferVK::GetGPUAddress()
    {
        vk::BufferDeviceAddressInfo addressInfo {};
        addressInfo.buffer = m_Buffer;

        return ((RHIDeviceVK*)m_pDevice)->GetDevice().getBufferAddress(addressInfo);
    }

    uint32_t RHIBufferVK::GetRequiredStagingBufferSize() const
    {
        vk::MemoryRequirements memReq {};
        ((RHIDeviceVK*)m_pDevice)->GetDevice().getBufferMemoryRequirements(m_Buffer, &memReq);
        return (uint32_t)memReq.size;
    }
}