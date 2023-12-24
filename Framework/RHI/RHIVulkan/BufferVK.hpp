#pragma once

#include "RHI/RHIBuffer.hpp"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class DeviceVK;

    class BufferVK : public RHIBuffer
    {
    public:
        NOCOPY(BufferVK)
        BufferVK(DeviceVK& inDevice, const BufferCreateInfo& createInfo);
        ~BufferVK();
        void Destroy() override;

        void* Map(RHIMapMode mapMode, size_t offset, size_t size) override;
        void Unmap() override;
        RHIBufferView* CreateBufferView(const BufferViewCreateInfo& createInfo) override;

        vk::Buffer GetVkBuffer() { return mBuffer; }
        RHIBufferUsageFlags GetUsages() { return mUsages; }

    private:
        void CreateBuffer(const BufferCreateInfo& createInfo);

    private:
        DeviceVK& mDevice;
        vk::Buffer mBuffer;
        VmaAllocation mAllocation;
        RHIBufferUsageFlags mUsages;
    };
} // namespace Vultana
