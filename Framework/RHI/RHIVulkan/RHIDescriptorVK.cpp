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
    }

    bool RHISamplerVK::Create()
    {
        return true;
    }
}