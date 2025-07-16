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

        virtual void* GetNativeHandle() const override { return m_Buffer; }
        virtual void* GetCPUAddress() override;
        virtual uint64_t GetGPUAddress() override;
        virtual uint32_t GetRequiredStagingBufferSize() const override;
    
    private:
        vk::Buffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        void* m_pData = nullptr;
    };
}