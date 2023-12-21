#pragma once

#include "RHI/RHIDevice.hpp"
#include "Utilities/Utility.hpp"

#include <vulkan/vulkan.hpp>

struct VmaAllocator_T;
struct VmaAllocation_T;
using VmaAllocator = VmaAllocator_T*;
using VmaAllocation = VmaAllocation_T*;

namespace Vultana
{
    class GPUVK;


    class DeviceVK : public RHIDevice
    {
    public:
        NOCOPY(DeviceVK)
        DeviceVK(GPUVK& gpu, const DeviceCreateInfo& createInfo);
        ~DeviceVK();

        void Destroy() override;
    };

    
} // namespace Vultana::RHI
