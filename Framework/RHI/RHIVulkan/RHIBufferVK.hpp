#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIBuffer.hpp"

namespace RHI
{
    class FVulkanDevice;

    class FVulkanBuffer : public FRHIBuffer
    {
    public:
        FVulkanBuffer(FVulkanDevice* device, const FRHIBufferDesc& desc, const eastl::string& name);
        ~FVulkanBuffer();

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