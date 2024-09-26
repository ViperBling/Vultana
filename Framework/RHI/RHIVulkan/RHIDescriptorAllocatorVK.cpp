#include "RHIDescriptorAllocatorVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Math.hpp"

namespace RHI
{
    RHIConstantBufferAllocatorVK::RHIConstantBufferAllocatorVK(RHIDeviceVK *device, uint32_t bufferSize)
    {
        mDevice = device;
        mBufferSize = bufferSize;

        vk::BufferCreateInfo bufferCI {};
        bufferCI.setSize(mBufferSize);
        bufferCI.setUsage(vk::BufferUsageFlagBits::eUniformBuffer | 
                         vk::BufferUsageFlagBits::eShaderDeviceAddress |
                         vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
        
        VmaAllocationCreateInfo allocCI {};
        allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        VmaAllocationInfo allocInfo {};
        vmaCreateBuffer(mDevice->GetVmaAllocator(), (VkBufferCreateInfo *)&bufferCI, &allocCI, (VkBuffer *)&mBuffer, &mAllocation, &allocInfo);

        mCPUAddress = allocInfo.pMappedData;

        vk::BufferDeviceAddressInfo bufferAddrInfo {};
        bufferAddrInfo.buffer = mBuffer;
        mGPUAddress = mDevice->GetDevice().getBufferAddress(&bufferAddrInfo);
    }

    RHIConstantBufferAllocatorVK::~RHIConstantBufferAllocatorVK()
    {
        vmaDestroyBuffer(mDevice->GetVmaAllocator(), mBuffer, mAllocation);
    }

    void RHIConstantBufferAllocatorVK::Allocate(uint32_t size, void **cpuAddress, vk::DeviceAddress *gpuAddress)
    {
        assert(mAllocatedSize + size <= mBufferSize);

        *cpuAddress = (char *)mCPUAddress + mAllocatedSize;
        *gpuAddress = mGPUAddress + mAllocatedSize;

        mAllocatedSize += RoundUpPow2(size, 256);
    }

    void RHIConstantBufferAllocatorVK::Reset()
    {
        mAllocatedSize = 0;
    }

    RHIDescriptorAllocatorVK::RHIDescriptorAllocatorVK(RHIDeviceVK *device, uint32_t descSize, uint32_t descCount, vk::BufferUsageFlags usage)
    {
        mDevice = device;
        mDescriptorSize = descSize;
        mDescriptorCount = descCount;

        vk::BufferCreateInfo bufferCI {};
        bufferCI.size = mDescriptorSize * mDescriptorCount;
        bufferCI.usage = usage | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        VmaAllocationCreateInfo allocCI {};
        allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        VmaAllocationInfo allocInfo {};
        vmaCreateBuffer(mDevice->GetVmaAllocator(), (VkBufferCreateInfo *)&bufferCI, &allocCI, (VkBuffer *)&mBuffer, &mAllocation, &allocInfo);

        mCPUAddress = allocInfo.pMappedData;

        vk::BufferDeviceAddressInfo bufferAddrInfo {};
        bufferAddrInfo.buffer = mBuffer;
        mGPUAddress = mDevice->GetDevice().getBufferAddress(&bufferAddrInfo);
    }

    RHIDescriptorAllocatorVK::~RHIDescriptorAllocatorVK()
    {
        vmaDestroyBuffer(mDevice->GetVmaAllocator(), mBuffer, mAllocation);
    }

    uint32_t RHIDescriptorAllocatorVK::Allocate(void **desc)
    {
        uint32_t index = 0;

        if (!mFreeDescriptors.empty())
        {
            index = mFreeDescriptors.back();
            mFreeDescriptors.pop_back();
        }
        else
        {
            assert(mAllocatedCount < mDescriptorCount);
            index = mAllocatedCount;
            ++mAllocatedCount;
        }
        *desc = (char *)mCPUAddress + mDescriptorSize * index;

        return index;
    }

    void RHIDescriptorAllocatorVK::Free(uint32_t index)
    {
        mFreeDescriptors.push_back(index);
    }
} // namespace RHI
