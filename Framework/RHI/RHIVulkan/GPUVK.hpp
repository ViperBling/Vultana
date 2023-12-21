#pragma once

#include <vulkan/vulkan.hpp>

#include "RHI/RHIGPU.hpp"

namespace Vultana
{
    class InstanceVK;

    class GPUVK : public RHIGPU
    {
    public:
        NOCOPY(GPUVK);
        explicit GPUVK(vk::Instance& instance, vk::PhysicalDevice& physicalDevice);
        ~GPUVK() = default;

        GPUProperty GetProperty() override;
        RHIDevice* RequestDevice(const DeviceCreateInfo& info) override;

        const vk::PhysicalDevice& GetVKPhysicalDevice() const { return mPhysicalDevice; }
        vk::Instance& GetInstance() const { return mInstance; }

        uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    private:
        vk::Instance& mInstance;
        vk::PhysicalDevice mPhysicalDevice;
    };
}