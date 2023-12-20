#include "VKGPU.hpp"

namespace Vultana
{
    VKGPU::VKGPU(vk::Instance &instance, vk::PhysicalDevice &physicalDevice)
        : RHIGPU()
        , mInstance(instance)
        , mPhysicalDevice(physicalDevice)
    {
    }

    GPUProperty VKGPU::GetProperty()
    {
        return GPUProperty();
    }

    RHIDevice *VKGPU::RequestDevice(const DeviceCreateInfo &info)
    {
        return nullptr;
    }

    const vk::PhysicalDevice &VKGPU::GetVKPhysicalDevice() const
    {
        // TODO: insert return statement here
    }

    vk::Instance &VKGPU::GetInstance() const
    {
        // TODO: insert return statement here
    }

    uint32_t VKGPU::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
    {
        return 0;
    }

} // namespace Vultana
