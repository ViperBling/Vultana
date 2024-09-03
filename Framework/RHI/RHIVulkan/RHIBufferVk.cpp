#include "RHIBufferVK.hpp"
#include "RHIDeviceVK.hpp"

namespace RHI
{
    RHIBufferVK::RHIBufferVK(RHIDeviceVK *device, const RHIBufferDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
    }

    RHIBufferVK::~RHIBufferVK()
    {
        
    }
}