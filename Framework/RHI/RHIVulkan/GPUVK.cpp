#include "GPUVK.hpp"
#include "DeviceVK.hpp"

namespace Vultana
{
    GPUVK::GPUVK(vk::Instance &instance, vk::PhysicalDevice &physicalDevice)
        : RHIGPU()
        , mInstance(instance)
        , mPhysicalDevice(physicalDevice)
    {
    }

    GPUProperty GPUVK::GetProperty()
    {
        return GPUProperty();
    }

    RHIDevice *GPUVK::RequestDevice(const DeviceCreateInfo &info)
    {
        return new DeviceVK(*this, info);
    }

    uint32_t GPUVK::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        return 0;
    }

} // namespace Vultana
