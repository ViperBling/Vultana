#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VKDevice.hpp"

namespace Vultana
{
    VKDevice::VKDevice(const RHIDeviceInfo &deviceInfo)
    {
        mDeviceInfo = deviceInfo;
    }
}