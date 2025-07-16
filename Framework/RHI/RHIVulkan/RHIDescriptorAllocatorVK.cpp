#include "RHIDescriptorAllocatorVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Math.hpp"

namespace RHI
{
    RHIConstantBufferAllocatorVK::RHIConstantBufferAllocatorVK(RHIDeviceVK *device, uint32_t bufferSize)
    {
        m_Device = device;
        m_BufferSize = bufferSize;

        vk::BufferCreateInfo bufferCI {};
        bufferCI.setSize(m_BufferSize);
        bufferCI.setUsage(vk::BufferUsageFlagBits::eUniformBuffer | 
                         vk::BufferUsageFlagBits::eShaderDeviceAddress |
                         vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
        
        VmaAllocationCreateInfo allocCI {};
        allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        VmaAllocationInfo allocInfo {};
        vmaCreateBuffer(m_Device->GetVmaAllocator(), (VkBufferCreateInfo *)&bufferCI, &allocCI, (VkBuffer *)&m_Buffer, &m_Allocation, &allocInfo);

        m_CPUAddress = allocInfo.pMappedData;

        vk::BufferDeviceAddressInfo bufferAddrInfo {};
        bufferAddrInfo.buffer = m_Buffer;
        m_GPUAddress = m_Device->GetDevice().getBufferAddress(&bufferAddrInfo);
    }

    RHIConstantBufferAllocatorVK::~RHIConstantBufferAllocatorVK()
    {
        vmaDestroyBuffer(m_Device->GetVmaAllocator(), m_Buffer, m_Allocation);
    }

    void RHIConstantBufferAllocatorVK::Allocate(uint32_t size, void **cpuAddress, vk::DeviceAddress *gpuAddress)
    {
        assert(m_AllocatedSize + size <= m_BufferSize);

        *cpuAddress = (char *)m_CPUAddress + m_AllocatedSize;
        *gpuAddress = m_GPUAddress + m_AllocatedSize;

        m_AllocatedSize += RoundUpPow2(size, 256);
    }

    void RHIConstantBufferAllocatorVK::Reset()
    {
        m_AllocatedSize = 0;
    }

    RHIDescriptorAllocatorVK::RHIDescriptorAllocatorVK(RHIDeviceVK *device, uint32_t descSize, uint32_t descCount, vk::BufferUsageFlags usage)
    {
        m_Device = device;
        m_DescriptorSize = descSize;
        m_DescriptorCount = descCount;

        vk::BufferCreateInfo bufferCI {};
        bufferCI.size = m_DescriptorSize * m_DescriptorCount;
        bufferCI.usage = usage | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        VmaAllocationCreateInfo allocCI {};
        allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        VmaAllocationInfo allocInfo {};
        vmaCreateBuffer(m_Device->GetVmaAllocator(), (VkBufferCreateInfo *)&bufferCI, &allocCI, (VkBuffer *)&m_Buffer, &m_Allocation, &allocInfo);

        m_CPUAddress = allocInfo.pMappedData;

        vk::BufferDeviceAddressInfo bufferAddrInfo {};
        bufferAddrInfo.buffer = m_Buffer;
        m_GPUAddress = m_Device->GetDevice().getBufferAddress(&bufferAddrInfo);
    }

    RHIDescriptorAllocatorVK::~RHIDescriptorAllocatorVK()
    {
        vmaDestroyBuffer(m_Device->GetVmaAllocator(), m_Buffer, m_Allocation);
    }

    uint32_t RHIDescriptorAllocatorVK::Allocate(void **desc)
    {
        uint32_t index = 0;

        if (!m_FreeDescriptors.empty())
        {
            index = m_FreeDescriptors.back();
            m_FreeDescriptors.pop_back();
        }
        else
        {
            assert(m_AllocatedCount < m_DescriptorCount);
            index = m_AllocatedCount;
            ++m_AllocatedCount;
        }
        *desc = (char *)m_CPUAddress + m_DescriptorSize * index;

        return index;
    }

    void RHIDescriptorAllocatorVK::Free(uint32_t index)
    {
        m_FreeDescriptors.push_back(index);
    }
} // namespace RHI
