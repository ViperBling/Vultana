#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIDescriptor.hpp"
#include "RHI/RHIBuffer.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHIShaderResourceViewVK : public RHIDescriptor
    {
    public:
        RHIShaderResourceViewVK(RHIDeviceVK* device, RHIResource* pResource, const RHIShaderResourceViewDesc& desc, const eastl::string& name);
        ~RHIShaderResourceViewVK();

        bool Create();
        vk::ImageView GetImageView() const { return m_ImageView; }

        const RHIShaderResourceViewDesc& GetDesc() const { return m_Desc; }
        virtual void* GetNativeHandle() const override { return m_Resource->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return m_HeapIndex; }
    
    private:
        RHIResource* m_Resource = nullptr;
        RHIShaderResourceViewDesc m_Desc {};
        vk::ImageView m_ImageView = VK_NULL_HANDLE;
        uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
    };

    class RHIUnorderedAccessViewVK : public RHIDescriptor
    {
    public:
        RHIUnorderedAccessViewVK(RHIDeviceVK* device, RHIResource* pResource, const RHIUnorderedAccessViewDesc& desc, const eastl::string& name);
        ~RHIUnorderedAccessViewVK();

        bool Create();

        const RHIUnorderedAccessViewDesc& GetDesc() const { return m_Desc; }
        virtual void* GetNativeHandle() const override { return m_Resource->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return m_HeapIndex; }

    private:
        RHIResource* m_Resource = nullptr;
        RHIUnorderedAccessViewDesc m_Desc {};
        vk::ImageView m_ImageView = VK_NULL_HANDLE;
        uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
    };

    class RHIConstantBufferViewVK : public RHIDescriptor
    {
    public:
        RHIConstantBufferViewVK(RHIDeviceVK* device, RHIBuffer* buffer, const RHIConstantBufferViewDesc& desc, const eastl::string& name);
        ~RHIConstantBufferViewVK();

        bool Create();

        const RHIConstantBufferViewDesc& GetDesc() const { return m_Desc; }
        virtual void* GetNativeHandle() const override { return m_Buffer->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return m_HeapIndex; }

    private:
        RHIBuffer* m_Buffer = nullptr;
        RHIConstantBufferViewDesc m_Desc {};
        uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
    };

    class RHISamplerVK : public RHIDescriptor
    {
    public:
        RHISamplerVK(RHIDeviceVK* device, const RHISamplerDesc& desc, const eastl::string& name);
        ~RHISamplerVK();

        bool Create();

        const RHISamplerDesc& GetDesc() const { return m_Desc; }
        virtual void* GetNativeHandle() const override { return m_Sampler; }
        virtual uint32_t GetHeapIndex() const override { return m_HeapIndex; }
    
    private:
        RHISamplerDesc m_Desc {};
        vk::Sampler m_Sampler = VK_NULL_HANDLE;
        uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
    };
}