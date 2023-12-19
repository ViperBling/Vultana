#include "VKDevice.hpp"

namespace Vultana
{
    VKDevice::VKDevice(const RHIDeviceInfo &deviceInfo)
    {
        mDeviceInfo = deviceInfo;
    }
    
    void VKDevice::OnCreate()
    {
    }

    void VKDevice::OnDestroy()
    {
    }
}