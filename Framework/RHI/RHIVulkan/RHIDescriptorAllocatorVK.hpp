#pragma once

#include "RHICommonVK.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHIConstantBufferAllocatorVK
    {
    public:
        RHIConstantBufferAllocatorVK(RHIDeviceVK* device, uint32_t bufferSize);
        ~RHIConstantBufferAllocatorVK();

        void Allocate(uint32_t size, void** cpuAddress, vk::DeviceAddress* gpuAddress);
        void Reset();

        vk::DeviceAddress GetGPUAddress() const { return m_GPUAddress; }

    
    private:
        RHIDeviceVK* m_Device = nullptr;
        vk::Buffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        vk::DeviceAddress m_GPUAddress = 0;
        void* m_CPUAddress = nullptr;
        uint32_t m_BufferSize = 0;
        uint32_t m_AllocatedSize = 0;
    };

    class RHIDescriptorAllocatorVK
    {
    public:
        RHIDescriptorAllocatorVK(RHIDeviceVK* device, uint32_t descSize, uint32_t descCount, vk::BufferUsageFlags usage);
        ~RHIDescriptorAllocatorVK();

        uint32_t Allocate(void** desc);
        void Free(uint32_t index);

        vk::DeviceAddress GetGPUAddress() const { return m_GPUAddress; }
    
    private:
        RHIDeviceVK* m_Device = nullptr;
        vk::Buffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        vk::DeviceAddress m_GPUAddress = 0;
        void* m_CPUAddress = nullptr;
        uint32_t m_DescriptorSize = 0;
        uint32_t m_DescriptorCount = 0;

        uint32_t m_AllocatedCount = 0;
        eastl::vector<uint32_t> m_FreeDescriptors;
    };
}

