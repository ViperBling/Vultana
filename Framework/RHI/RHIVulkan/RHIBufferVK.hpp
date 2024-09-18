#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIBuffer.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHIBufferVK : public RHIBuffer
    {
    public:
        RHIBufferVK(RHIDeviceVK* device, const RHIBufferDesc& desc, const std::string& name);
        ~RHIBufferVK();

        bool Create();

        virtual void* GetNativeHandle() const override { return mBuffer; }
        virtual void* GetCPUAddress() const override;
        virtual uint64_t GetGPUAddress() const override;
        virtual uint32_t GetRequiredStagingBufferSize() const override;

        vk::Buffer GetBuffer() const { return mBuffer; }
        VmaAllocation GetAllocation() const { return mAllocation; }
    
    private:
        vk::Buffer mBuffer = VK_NULL_HANDLE;
        VmaAllocation mAllocation = VK_NULL_HANDLE;
        void* mpData = nullptr;
    };
}