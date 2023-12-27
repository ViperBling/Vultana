#pragma once

#include "RHI/RHIBindGroup.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class DeviceVK;

    class BindGroupVK : public RHIBindGroup
    {
    public:
        NOCOPY(BindGroupVK)
        BindGroupVK(DeviceVK& device, const BindGroupCreateInfo& createInfo);
        ~BindGroupVK();
        void Destroy() override;

        vk::DescriptorSet GetVkDescriptorSet() const { return mDescriptorSet; }

    private:
        void CreateDescriptorSet(const BindGroupCreateInfo& createInfo);
        void CreateDescriptorPool(const BindGroupCreateInfo& createInfo);

    private:
        DeviceVK& mDevice;
        vk::DescriptorSet mDescriptorSet = VK_NULL_HANDLE;
        vk::DescriptorPool mDescriptorPool;
    };
}