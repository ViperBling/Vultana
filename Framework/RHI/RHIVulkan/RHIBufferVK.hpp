#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIBuffer.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHIBufferVK : public RHIBuffer
    {
    public:
        RHIBufferVK(RHIDeviceVK* device, const RHIBufferDesc& desc, const eastl::string& name);
        ~RHIBufferVK();

        bool Create();

        virtual void* GetNativeHandle() const override { return mBuffer; }
        virtual void* GetCPUAddress() override;
        virtual uint64_t GetGPUAddress() override;
        virtual uint32_t GetRequiredStagingBufferSize() const override;
    
    private:
        vk::Buffer mBuffer = VK_NULL_HANDLE;
        VmaAllocation mAllocation = VK_NULL_HANDLE;
        void* mpData = nullptr;
    };
}