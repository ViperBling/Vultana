#include "RHIDescriptorVK.hpp"
#include "RHIDeviceVK.hpp"

namespace RHI
{
    RHIShaderResourceViewVK::RHIShaderResourceViewVK(RHIDeviceVK *device, RHIResource *pResource, const RHIShaderResourceViewDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mName = name;
        mResource = pResource;
        mDesc = desc;
    }

    RHIShaderResourceViewVK::~RHIShaderResourceViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(mpDevice);
        device->Delete(mImageView);
        device->FreeResourceDescriptor(mHeapIndex);
    }

    bool RHIShaderResourceViewVK::Create()
    {
        return true;
    }

    RHIUnorderedAccessViewVK::RHIUnorderedAccessViewVK(RHIDeviceVK *device, RHIResource *pResource, const RHIUnorderedAccessViewDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mName = name;
        mResource = pResource;
        mDesc = desc;
    }

    RHIUnorderedAccessViewVK::~RHIUnorderedAccessViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(mpDevice);
        device->Delete(mImageView);
        device->FreeResourceDescriptor(mHeapIndex);
    }

    bool RHIUnorderedAccessViewVK::Create()
    {
        return true;
    }

    RHIConstantBufferViewVK::RHIConstantBufferViewVK(RHIDeviceVK *device, RHIBuffer *buffer, const RHIConstantBufferViewDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mName = name;
        mBuffer = buffer;
        mDesc = desc;
    }

    RHIConstantBufferViewVK::~RHIConstantBufferViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(mpDevice);
        device->FreeResourceDescriptor(mHeapIndex);
    }

    bool RHIConstantBufferViewVK::Create()
    {
        return true;
    }

    RHISamplerVK::RHISamplerVK(RHIDeviceVK *device, const RHISamplerDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mName = name;
        mDesc = desc;
    }

    RHISamplerVK::~RHISamplerVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(mpDevice);
        device->Delete(mSampler);
        device->FreeResourceDescriptor(mHeapIndex);
    }

    bool RHISamplerVK::Create()
    {
        return true;
    }
}