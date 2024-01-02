#pragma once

#include <vulkan/vulkan.hpp>

#include "RHI/RHIGPU.hpp"

namespace RHI
{
    class InstanceVK;

    class GPUVK : public RHIGPU
    {
    public:
        NOCOPY(GPUVK);
        explicit GPUVK(InstanceVK& instance, vk::PhysicalDevice physicalDevice);
        ~GPUVK() = default;

        GPUProperty GetProperty() override;
        RHIDevice* RequestDevice(const DeviceCreateInfo& info) override;

        const vk::PhysicalDevice& GetVkPhysicalDevice() const { return mPhysicalDevice; }
        InstanceVK& GetInstance() const { return mInstance; }

        uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    private:
        InstanceVK& mInstance;
        vk::PhysicalDevice mPhysicalDevice;
    };
}