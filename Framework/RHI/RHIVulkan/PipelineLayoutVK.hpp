#pragma once

#include "RHI/RHIPipelineLayout.hpp"

#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class DeviceVK;

    class PipelineLayoutVK : public RHIPipelineLayout
    {
    public:
        NOCOPY(PipelineLayoutVK);
        PipelineLayoutVK(DeviceVK& device, const PipelineLayoutCreateInfo& createInfo);
        ~PipelineLayoutVK();
        void Destroy() override;

        vk::PipelineLayout GetVkPipelineLayout() const { return mPipelineLayout; }

    private:
        void CreateNativePipelineLayout(const PipelineLayoutCreateInfo& createInfo);

    private:
        DeviceVK& mDevice;
        vk::PipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    };
}