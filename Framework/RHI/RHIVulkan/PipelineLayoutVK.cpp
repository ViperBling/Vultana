#include "PipelineLayoutVK.hpp"

#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"
#include "BindGroupLayoutVK.hpp"

namespace Vultana
{
    PipelineLayoutVK::PipelineLayoutVK(DeviceVK &device, const PipelineLayoutCreateInfo &createInfo)
        : RHIPipelineLayout(createInfo)
        , mDevice(device)
    {
        CreateNativePipelineLayout(createInfo);
    }

    PipelineLayoutVK::~PipelineLayoutVK()
    {
    }

    void PipelineLayoutVK::Destroy()
    {
        if (mPipelineLayout) { mDevice.GetVkDevice().destroyPipelineLayout(mPipelineLayout); }
    }

    void PipelineLayoutVK::CreateNativePipelineLayout(const PipelineLayoutCreateInfo &createInfo)
    {
        std::vector<vk::DescriptorSetLayout> descSetLayouts(createInfo.BindGroupLayoutCount);
        for (uint32_t i = 0; i < createInfo.BindGroupLayoutCount; ++i)
        {
            const auto* bindGroupVK = static_cast<const BindGroupLayoutVK*>(createInfo.BindGroupLayouts[i]);
            descSetLayouts[i] = bindGroupVK->GetVkDescriptorSetLayout();
        }

        std::vector<vk::PushConstantRange> pusConstants(createInfo.PipelineConstantLayoutCount);
        for (uint32_t i = 0; i < createInfo.PipelineConstantLayoutCount; ++i)
        {
            const auto& constantInfo = createInfo.PipelineConstantLayouts[i];
            pusConstants[i].setStageFlags(GetVkShaderStageFlags(constantInfo.ShaderStage));
            pusConstants[i].setOffset(constantInfo.Offset);
            pusConstants[i].setSize(constantInfo.Size);
        }

        vk::PipelineLayoutCreateInfo pipelineCI {};
        pipelineCI.setSetLayouts(descSetLayouts);
        pipelineCI.setPushConstantRanges(pusConstants);

        mPipelineLayout = mDevice.GetVkDevice().createPipelineLayout(pipelineCI);

        if (!createInfo.Name.empty())
        {
            mDevice.SetObjectName(vk::ObjectType::ePipelineLayout, reinterpret_cast<uint64_t>(VkPipelineLayout(mPipelineLayout)), createInfo.Name.c_str());
        }
    }
}