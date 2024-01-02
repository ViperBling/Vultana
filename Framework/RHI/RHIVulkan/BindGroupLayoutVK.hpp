#pragma once

#include "RHI/RHIBindGroupLayout.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    class DeviceVK;

    class BindGroupLayoutVK : public RHIBindGroupLayout
    {
    public:
        NOCOPY(BindGroupLayoutVK);
        BindGroupLayoutVK(DeviceVK& device, const BindGroupLayoutCreateInfo& createInfo);
        ~BindGroupLayoutVK();
        void Destroy() override;

        vk::DescriptorSetLayout GetVkDescriptorSetLayout() const { return mDescriptorSetLayout; }

    private:
        void CreateDescriptorSetLayout(const BindGroupLayoutCreateInfo& createInfo);

    private:
        DeviceVK& mDevice;
        vk::DescriptorSetLayout mDescriptorSetLayout;
    };
}