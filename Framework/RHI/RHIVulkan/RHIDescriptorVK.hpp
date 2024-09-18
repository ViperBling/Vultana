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
        RHIShaderResourceViewVK(RHIDeviceVK* device, RHIResource* pResource, const RHIShaderResourceViewDesc& desc, const std::string& name);
        ~RHIShaderResourceViewVK();

        bool Create();

        virtual void* GetNativeHandle() const override { return mResource->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return mHeapIndex; }
    
    private:
        RHIResource* mResource = nullptr;
        RHIShaderResourceViewDesc mDesc {};
        vk::ImageView mImageView;
        uint32_t mHeapIndex = RHI_INVALID_RESOURCE;
    };

    class RHIUnorderedAccessViewVK : public RHIDescriptor
    {
    public:
        RHIUnorderedAccessViewVK(RHIDeviceVK* device, RHIResource* pResource, const RHIUnorderedAccessViewDesc& desc, const std::string& name);
        ~RHIUnorderedAccessViewVK();

        bool Create();

        virtual void* GetNativeHandle() const override { return mResource->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return mHeapIndex; }

    private:
        RHIResource* mResource = nullptr;
        RHIUnorderedAccessViewDesc mDesc {};
        vk::ImageView mImageView;
        uint32_t mHeapIndex = RHI_INVALID_RESOURCE;
    };

    class RHIConstantBufferViewVK : public RHIDescriptor
    {
    public:
        RHIConstantBufferViewVK(RHIDeviceVK* device, RHIBuffer* buffer, const RHIConstantBufferViewDesc& desc, const std::string& name);
        ~RHIConstantBufferViewVK();

        bool Create();

        virtual void* GetNativeHandle() const override { return mBuffer->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return mHeapIndex; }

    private:
        RHIBuffer* mBuffer = nullptr;
        RHIConstantBufferViewDesc mDesc {};
        uint32_t mHeapIndex = RHI_INVALID_RESOURCE;
    };

    class RHISamplerVK : public RHIDescriptor
    {
    public:
        RHISamplerVK(RHIDeviceVK* device, const RHISamplerDesc& desc, const std::string& name);
        ~RHISamplerVK();

        bool Create();

        virtual void* GetNativeHandle() const override { return mSampler; }
        virtual uint32_t GetHeapIndex() const override { return mHeapIndex; }
    
    private:
        RHISamplerDesc mDesc {};
        vk::Sampler mSampler;
        uint32_t mHeapIndex = RHI_INVALID_RESOURCE;
    };
}