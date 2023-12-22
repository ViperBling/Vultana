#include "RHICommonVK.hpp"
#include "GPUVK.hpp"
#include "DeviceVK.hpp"

namespace Vultana
{
    GPUVK::GPUVK(InstanceVK& instance, vk::PhysicalDevice physicalDevice)
        : RHIGPU()
        , mInstance(instance)
        , mPhysicalDevice(physicalDevice)
    {
    }

    GPUProperty GPUVK::GetProperty()
    {
        vk::PhysicalDeviceProperties properties = mPhysicalDevice.getProperties();

        GPUProperty property;
        property.VendorID = properties.vendorID;
        property.DeviceID = properties.deviceID;
        property.Type = VKEnumCast<vk::PhysicalDeviceType, RHIDeviceType>(properties.deviceType);
        return property;
    }

    RHIDevice* GPUVK::RequestDevice(const DeviceCreateInfo &info)
    {
        return new DeviceVK(*this, info);
    }

    uint32_t GPUVK::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        vk::PhysicalDeviceMemoryProperties memProperties = mPhysicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }
        
        return -1;
    }

} // namespace Vultana
