#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIDescriptor.hpp"
#include "RHI/RHIBuffer.hpp"

namespace RHI
{
    class FVulkanDevice;

    class FVulkanShaderResourceView : public FRHIDescriptor
    {
    public:
        FVulkanShaderResourceView(FVulkanDevice* device, FRHIResource* pResource, const FRHIShaderResourceViewDesc& desc, const eastl::string& name);
        ~FVulkanShaderResourceView();

        bool Create();
        vk::ImageView GetImageView() const { return m_ImageView; }

        const FRHIShaderResourceViewDesc& GetDesc() const { return m_Desc; }
        virtual void* GetNativeHandle() const override { return m_Resource->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return m_HeapIndex; }
    
    private:
        FRHIResource* m_Resource = nullptr;
        FRHIShaderResourceViewDesc m_Desc {};
        vk::ImageView m_ImageView = VK_NULL_HANDLE;
        uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
    };

    class FVulkanUnorderedAccessView : public FRHIDescriptor
    {
    public:
        FVulkanUnorderedAccessView(FVulkanDevice* device, FRHIResource* pResource, const FRHIUnorderedAccessViewDesc& desc, const eastl::string& name);
        ~FVulkanUnorderedAccessView();

        bool Create();

        const FRHIUnorderedAccessViewDesc& GetDesc() const { return m_Desc; }
        virtual void* GetNativeHandle() const override { return m_Resource->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return m_HeapIndex; }

    private:
        FRHIResource* m_Resource = nullptr;
        FRHIUnorderedAccessViewDesc m_Desc {};
        vk::ImageView m_ImageView = VK_NULL_HANDLE;
        uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
    };

    class FVulkanConstantBufferView : public FRHIDescriptor
    {
    public:
        FVulkanConstantBufferView(FVulkanDevice* device, FRHIBuffer* buffer, const FRHIConstantBufferViewDesc& desc, const eastl::string& name);
        ~FVulkanConstantBufferView();

        bool Create();

        const FRHIConstantBufferViewDesc& GetDesc() const { return m_Desc; }
        virtual void* GetNativeHandle() const override { return m_Buffer->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return m_HeapIndex; }

    private:
        FRHIBuffer* m_Buffer = nullptr;
        FRHIConstantBufferViewDesc m_Desc {};
        uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
    };

    class FVulkanSampler : public FRHIDescriptor
    {
    public:
        FVulkanSampler(FVulkanDevice* device, const FRHISamplerDesc& desc, const eastl::string& name);
        ~FVulkanSampler();

        bool Create();

        const FRHISamplerDesc& GetDesc() const { return m_Desc; }
        virtual void* GetNativeHandle() const override { return m_Sampler; }
        virtual uint32_t GetHeapIndex() const override { return m_HeapIndex; }
    
    private:
        FRHISamplerDesc m_Desc {};
        vk::Sampler m_Sampler = VK_NULL_HANDLE;
        uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
    };
}