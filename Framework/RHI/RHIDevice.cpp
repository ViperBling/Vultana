#include "RHIDevice.hpp"
#include "Vulkan/VKDevice.hpp"

namespace Vultana::RHI
{
    RHIDevice* CreateRHIDevice(const RHIDeviceInfo &deviceInfo)
    {
        RHIDevice* pDevice = nullptr;

        switch (deviceInfo.Backend)
        {
        case RHI::RHIRenderBackend::Vulkan:
            pDevice = new VKDevice(deviceInfo);
            if (!(VKDevice*)pDevice->Init())
            {
                delete pDevice;
                pDevice = nullptr;
            }
            break;
        default:
            break;
        }
        return pDevice;
    }
} // namespace Vultana::RHI
