#include "SurfaceVK.hpp"
#include "InstanceVK.hpp"
#include "GPUVK.hpp"
#include "DeviceVK.hpp"

namespace Vultana
{
    SurfaceVK::SurfaceVK(DeviceVK &inDevice, const SurfaceCreateInfo &createInfo)
        : RHISurface(createInfo)
        , mDevice(inDevice)
    {
    }

    SurfaceVK::~SurfaceVK()
    {
        Destroy();
    }

    void SurfaceVK::Destroy()
    {
        if (mSurface)
        {
            mDevice.GetGPU().GetInstance().GetVkInstance().destroySurfaceKHR(mSurface);
        }
    }
} // namespace Vultana
