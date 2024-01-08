#pragma once

#include "RHI/RHIBuffer.hpp"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace RHI::Vulkan
{
    class RHIDeviceVK;

    class RHIBufferVK : public RHIBuffer
    {
    public:
        RHIBufferVK(RHIDeviceVK* device, const RHIBufferDesc& desc, const std::string& name);
        ~RHIBufferVK();

        virtual void* GetNativeHandle() const override;
        virtual void* GetCPUAddress() const override;
        virtual uint64_t GetGPUAddress() const override { return 0; }
        virtual uint32_t GetRequiredStagingBufferSize() const override;

        bool Create();

        vk::Buffer GetBuffer() const { return mBuffer; }
        VmaAllocation GetAllocation() const { return mAllocation; }
    
    private:
        vk::Buffer mBuffer;
        VmaAllocation mAllocation;
        void* mpData = nullptr;
    };
}