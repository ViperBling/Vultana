#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VKDevice.hpp"

using namespace Vultana;

namespace Vultana::RHI
{
    VKDevice::VKDevice(const RHIDeviceInfo &deviceInfo)
    {
        mDeviceInfo = deviceInfo;
    }
}