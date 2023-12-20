#pragma once

#include <vulkan/vulkan.hpp>

#include "RHI/RHIGPU.hpp"

namespace Vultana
{
    class VKInstance;

    class VKGPU : public RHIGPU
    {
    public:
        NOCOPY(VKGPU);
        explicit VKGPU(vk::Instance& instance, vk::PhysicalDevice& physicalDevice);
        ~VKGPU() = default;

        GPUProperty GetProperty() override;
        RHIDevice* RequestDevice(const DeviceCreateInfo& info) override;

        const vk::PhysicalDevice& GetVKPhysicalDevice() const;
        vk::Instance& GetInstance() const;

        uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    private:
        vk::Instance& mInstance;
        vk::PhysicalDevice mPhysicalDevice;
    };
}