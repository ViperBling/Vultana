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

        bool Creat();

        virtual void* GetNativeHandle() const override { return mResource->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return 0; }
    
    private:
        RHIResource* mResource = nullptr;
        RHIShaderResourceViewDesc mDesc {};
    };

    class RHIUnorderedAccessViewVK : public RHIDescriptor
    {
    public:
        RHIUnorderedAccessViewVK(RHIDeviceVK* device, RHIResource* pResource, const RHIUnorderedAccessViewDesc& desc, const std::string& name);
        ~RHIUnorderedAccessViewVK();

        bool Creat();

        virtual void* GetNativeHandle() const override { return mResource->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return 0; }

    private:
        RHIResource* mResource = nullptr;
        RHIUnorderedAccessViewDesc mDesc {};
    };

    class RHIConstantBufferViewVK : public RHIDescriptor
    {
    public:
        RHIConstantBufferViewVK(RHIDeviceVK* device, RHIBuffer* buffer, const RHIConstantBufferViewDesc& desc, const std::string& name);
        ~RHIConstantBufferViewVK();

        bool Creat();

        virtual void* GetNativeHandle() const override { return mBuffer->GetNativeHandle(); }
        virtual uint32_t GetHeapIndex() const override { return 0; }

    private:
        RHIBuffer* mBuffer = nullptr;
        RHIConstantBufferViewDesc mDesc {};
    };

    class RHISamplerVK : public RHIDescriptor
    {
    public:
        RHISamplerVK(RHIDeviceVK* device, const RHISamplerDesc& desc, const std::string& name);
        ~RHISamplerVK();

        bool Creat();

        virtual void* GetNativeHandle() const override { return mSampler; }
        virtual uint32_t GetHeapIndex() const override { return 0; }
    
    private:
        RHISamplerDesc mDesc {};
        vk::Sampler mSampler;
    };
}