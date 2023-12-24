#include "BufferVK.hpp"
#include "DeviceVK.hpp"

#include "RHICommonVK.hpp"
#include "BufferViewVK.hpp"

namespace Vultana
{
    BufferVK::BufferVK(DeviceVK &inDevice, const BufferCreateInfo &createInfo)
        : RHIBuffer(createInfo)
        , mDevice(inDevice)
        , mUsages(createInfo.Usage)
    {
    }

    BufferVK::~BufferVK()
    {
        Destroy();
    }

    void BufferVK::Destroy()
    {
        if (mBuffer)
        {
            vmaDestroyBuffer(mDevice.GetVmaAllocator(), mBuffer, mAllocation);
        }
    }

    void *BufferVK::Map(RHIMapMode mapMode, size_t offset, size_t size)
    {
        void * data;
        assert(vmaMapMemory(mDevice.GetVmaAllocator(), mAllocation, &data) == VK_SUCCESS);
        return data;
    }

    void BufferVK::Unmap()
    {
        vmaUnmapMemory(mDevice.GetVmaAllocator(), mAllocation);
    }

    RHIBufferView *BufferVK::CreateBufferView(const BufferViewCreateInfo &createInfo)
    {
        return new BufferViewVK(*this, createInfo);
    }

    void BufferVK::CreateBuffer(const BufferCreateInfo &createInfo)
    {
        vk::BufferCreateInfo bufferCI {};
        bufferCI.setSharingMode(vk::SharingMode::eExclusive);
        bufferCI.setUsage(GetVkBufferUsageFlags(createInfo.Usage));
        bufferCI.setSize(createInfo.Size);

        VmaAllocationCreateInfo allocCI {};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        if (createInfo.Usage | RHIBufferUsageBits::MapWrite)
        {
            allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        assert(vmaCreateBuffer(mDevice.GetVmaAllocator(), (VkBufferCreateInfo*)&bufferCI, &allocCI, (VkBuffer*)&mBuffer, &mAllocation, nullptr) == VK_SUCCESS);

        if (!createInfo.Name.empty())
        {
            mDevice.SetObjectName(vk::ObjectType::eBuffer, reinterpret_cast<uint64_t>(VkBuffer(mBuffer)), createInfo.Name.c_str());
        }
    }
}