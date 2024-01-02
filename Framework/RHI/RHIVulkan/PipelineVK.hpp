#pragma once

#include "RHI/RHIPipeline.hpp"

#include <vulkan/vulkan.hpp>

namespace RHI
{
    class DeviceVK;
    class PipelineLayoutVK;

    class GraphicsPipelineVK : public RHIGraphicsPipeline
    {
    public:
        NOCOPY(GraphicsPipelineVK);
        GraphicsPipelineVK(DeviceVK& device, const GraphicsPipelineCreateInfo& createInfo);
        ~GraphicsPipelineVK();
        void Destroy() override;

        PipelineLayoutVK* GetPipelineLayout() const;
        vk::Pipeline GetVkPipeline () { return mPipeline; }
        vk::RenderPass GetVkRenderPass() { return mRenderPass; }

    private:
        void SavePipelineLayout(const GraphicsPipelineCreateInfo& createInfo);
        void CreateNativeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
        void CreateNativeRenderPass(const GraphicsPipelineCreateInfo& createInfo);

    private:
        DeviceVK& mDevice;
        PipelineLayoutVK* mPipelineLayout = nullptr;
        vk::Pipeline mPipeline = VK_NULL_HANDLE;
        vk::RenderPass mRenderPass = VK_NULL_HANDLE;
    };
}