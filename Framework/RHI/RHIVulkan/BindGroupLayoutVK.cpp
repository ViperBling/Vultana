#include "BindGroupLayoutVK.hpp"

#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"

namespace RHI
{
    BindGroupLayoutVK::BindGroupLayoutVK(DeviceVK &device, const BindGroupLayoutCreateInfo &createInfo)
        : RHIBindGroupLayout(createInfo)
        , mDevice(device)
    {
        CreateDescriptorSetLayout(createInfo);
    }

    BindGroupLayoutVK::~BindGroupLayoutVK()
    {
        Destroy();
    }

    void BindGroupLayoutVK::Destroy()
    {
        if (mDescriptorSetLayout) { mDevice.GetVkDevice().destroyDescriptorSetLayout(mDescriptorSetLayout); }
    }

    void BindGroupLayoutVK::CreateDescriptorSetLayout(const BindGroupLayoutCreateInfo &createInfo)
    {
        vk::DescriptorSetLayoutCreateInfo descSetLayoutCI {};

        std::vector<vk::DescriptorSetLayoutBinding> descBindings(createInfo.EntryCount);
        for (size_t i = 0; i < createInfo.EntryCount; i++)
        {
            auto& entry = createInfo.Entries[i];
            auto& binding = descBindings[i];

            vk::ShaderStageFlags ssFlags = GetVkShaderStageFlags(entry.ShaderStage);
            binding.setDescriptorType(VKEnumCast<RHIBindingType, vk::DescriptorType>(entry.Binding.Type));
            binding.setDescriptorCount(1);
            binding.setBinding(entry.Binding.Platform.GLSL.Index);
            binding.setStageFlags(ssFlags);
        }
        descSetLayoutCI.setBindings(descBindings);

        mDescriptorSetLayout = mDevice.GetVkDevice().createDescriptorSetLayout(descSetLayoutCI);

        if (!createInfo.Name.empty())
        {
            mDevice.SetObjectName(vk::ObjectType::eDescriptorSetLayout, reinterpret_cast<uint64_t>(VkDescriptorSetLayout(mDescriptorSetLayout)), createInfo.Name.c_str());
        }
    }
}