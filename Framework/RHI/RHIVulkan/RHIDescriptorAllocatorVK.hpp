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

        vk::DeviceAddress GetGPUAddress() const { return mGPUAddress; }

    
    private:
        RHIDeviceVK* mDevice;
        vk::Buffer mBuffer;
        VmaAllocation mAllocation;
        vk::DeviceAddress mGPUAddress = 0;
        void* mCPUAddress = nullptr;
        uint32_t mBufferSize = 0;
        uint32_t mAllocatedSize = 0;
    };

    class RHIDescriptorAllocatorVK
    {
    public:
        RHIDescriptorAllocatorVK(RHIDeviceVK* device, uint32_t descSize, uint32_t descCount, vk::BufferUsageFlags usage);
        ~RHIDescriptorAllocatorVK();

        uint32_t Allocate(void** desc);
        void Free(uint32_t index);

        vk::DeviceAddress GetGPUAddress() const { return mGPUAddress; }
    
    private:
        RHIDeviceVK* mDevice = nullptr;
        vk::Buffer mBuffer;
        VmaAllocation mAllocation;
        vk::DeviceAddress mGPUAddress = 0;
        void* mCPUAddress = nullptr;
        uint32_t mDescriptorSize = 0;
        uint32_t mDescriptorCount = 0;

        uint32_t mAllocatedCount = 0;
        std::vector<uint32_t> mFreeDescriptors;
    };
}

