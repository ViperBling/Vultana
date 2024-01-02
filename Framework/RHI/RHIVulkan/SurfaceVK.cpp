// #if (WIN32)
// #define VK_USE_PLATFORM_WIN32_KHR
// #endif

#include "SurfaceVK.hpp"
#include "InstanceVK.hpp"
#include "GPUVK.hpp"
#include "DeviceVK.hpp"

// #include <vulkan/vulkan_win32.h>
// #include <Windows.h>

#include "Windows/GLFWindow.hpp"

namespace RHI
{
    SurfaceVK::SurfaceVK(DeviceVK &inDevice, const SurfaceCreateInfo &createInfo)
        : RHISurface(createInfo)
        , mDevice(inDevice)
    {
        mSurface = Window::CreateVulkanSurface((GLFWwindow*)createInfo.Window, mDevice.GetGPU().GetInstance().GetVkInstance());
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
