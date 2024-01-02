#include "BindGroupVK.hpp"

#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"
#include "BindGroupLayoutVK.hpp"
#include "BufferVK.hpp"
#include "BufferViewVK.hpp"
#include "TextureVK.hpp"
#include "TextureViewVK.hpp"
#include "SamplerVK.hpp"

namespace RHI
{
    BindGroupVK::BindGroupVK(DeviceVK &device, const BindGroupCreateInfo &createInfo)
        : RHIBindGroup(createInfo)
        , mDevice(device)
    {
        CreateDescriptorPool(createInfo);
        CreateDescriptorSet(createInfo);
    }

    BindGroupVK::~BindGroupVK()
    {
        Destroy();
    }

    void BindGroupVK::Destroy()
    {
        if (mDescriptorPool) { mDevice.GetVkDevice().destroyDescriptorPool(mDescriptorPool); }
    }

    void BindGroupVK::CreateDescriptorSet(const BindGroupCreateInfo &createInfo)
    {
        vk::DescriptorSetLayout descSetLayout = dynamic_cast<BindGroupLayoutVK*>(createInfo.Layout)->GetVkDescriptorSetLayout();

        vk::DescriptorSetAllocateInfo descSetAI {};
        descSetAI.setDescriptorPool(mDescriptorPool);
        descSetAI.setSetLayouts(descSetLayout);

        mDescriptorSet = mDevice.GetVkDevice().allocateDescriptorSets(descSetAI).front();

        if (!createInfo.Name.empty())
        {
            mDevice.SetObjectName(vk::ObjectType::eDescriptorSet, reinterpret_cast<uint64_t>(VkDescriptorSet(mDescriptorSet)), createInfo.Name.c_str());
        }

        std::vector<vk::WriteDescriptorSet> descWrites(createInfo.EntryCount);
        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::DescriptorImageInfo> imageInfos;
        
        int bufferInfosCount = 0;
        int imageInfosCount = 0;
        for (size_t i = 0; i < createInfo.EntryCount; i++)
        {
            auto& entry = createInfo.Entries[i];
            auto& write = descWrites[i];

            write.setDstSet(mDescriptorSet);
            write.setDstBinding(entry.Binding.Platform.GLSL.Index);
            write.setDstArrayElement(0);
            write.setDescriptorCount(1);
            write.setDescriptorType(VKEnumCast<RHIBindingType, vk::DescriptorType>(entry.Binding.Type));

            if (entry.Binding.Type == RHIBindingType::UniformBuffer)
            {
                auto* bufferView = dynamic_cast<BufferViewVK*>(entry.BufferView);

                bufferInfos.emplace_back();
                bufferInfos.back().buffer = bufferView->GetBuffer().GetVkBuffer();
                bufferInfos.back().offset = bufferView->GetOffset();
                bufferInfos.back().range = bufferView->GetBufferSize();

                write.setPBufferInfo(&bufferInfos.back());
            }
            else if (entry.Binding.Type == RHIBindingType::Sampler)
            {
                auto* sampler = dynamic_cast<SamplerVK*>(entry.Sampler);

                imageInfos.emplace_back();
                imageInfos.back().sampler = sampler->GetVkSampler();

                write.setPImageInfo(&imageInfos.back());
            }
            else if (entry.Binding.Type == RHIBindingType::Texture)
            {
                auto* textureView = dynamic_cast<TextureViewVK*>(entry.TextureView);

                imageInfos.emplace_back();
                imageInfos.back().imageView = textureView->GetVkImageView();
                imageInfos.back().imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

                write.setPImageInfo(&imageInfos.back());
            }
            else 
            {
                // TODO
            }
        }

        mDevice.GetVkDevice().updateDescriptorSets(descWrites, {});
    }

    void BindGroupVK::CreateDescriptorPool(const BindGroupCreateInfo &createInfo)
    {
        std::vector<vk::DescriptorPoolSize> poolSizes(createInfo.EntryCount);

        for (size_t i = 0; i < createInfo.EntryCount; i++)
        {
            auto& entry = createInfo.Entries[i];
            auto& poolSize = poolSizes[i];

            poolSize.type = VKEnumCast<RHIBindingType, vk::DescriptorType>(entry.Binding.Type);
            poolSize.descriptorCount = 1;
        }
        vk::DescriptorPoolCreateInfo descPoolCI {};
        descPoolCI.setPoolSizes(poolSizes);
        descPoolCI.setMaxSets(1);

        mDescriptorPool = mDevice.GetVkDevice().createDescriptorPool(descPoolCI);
    }

} // namespace Vultana
