#pragma once

#include "RHI/RHISurface.hpp"
#include "Utilities/Utility.hpp"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class DeviceVK;

    class SurfaceVK : public RHISurface
    {
    public:
        NOCOPY(SurfaceVK)
        SurfaceVK(DeviceVK& inDevice, const SurfaceCreateInfo& createInfo);
        ~SurfaceVK();

        void Destroy() override;

        vk::SurfaceKHR GetVkSurface() const { return mSurface; }

    private:
        DeviceVK& mDevice;
        vk::SurfaceKHR mSurface;
    };
}