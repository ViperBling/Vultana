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
    class VKDevice : public RHIDevice
    {
    
    };

    
} // namespace Vultana::RHI
