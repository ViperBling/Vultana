#include "SamplerVK.hpp"

#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"

namespace Vultana
{
    SamplerVK::SamplerVK(DeviceVK &device, const SamplerCreateInfo &createInfo)
        : RHISampler(createInfo)
        , mDevice(device)
    {
        CreateSampler(createInfo);
    }

    SamplerVK::~SamplerVK()
    {
    }

    void SamplerVK::Destroy()
    {
        if (mSampler)
        {
            mDevice.GetVkDevice().destroySampler(mSampler);
            mSampler = VK_NULL_HANDLE;
        }
    }

    void SamplerVK::CreateSampler(const SamplerCreateInfo &createInfo)
    {
        vk::SamplerCreateInfo samplerCI {};
        
        samplerCI.addressModeU = VKEnumCast<RHISamplerAddressMode, vk::SamplerAddressMode>(createInfo.AddressModeU);
        samplerCI.addressModeV = VKEnumCast<RHISamplerAddressMode, vk::SamplerAddressMode>(createInfo.AddressModeV);
        samplerCI.addressModeW = VKEnumCast<RHISamplerAddressMode, vk::SamplerAddressMode>(createInfo.AddressModeW);
        samplerCI.magFilter = VKEnumCast<RHISamplerFilterMode, vk::Filter>(createInfo.MagFilter);
        samplerCI.minFilter = VKEnumCast<RHISamplerFilterMode, vk::Filter>(createInfo.MinFilter);
        samplerCI.mipmapMode = VKEnumCast<RHISamplerFilterMode, vk::SamplerMipmapMode>(createInfo.MipmapMode);
        samplerCI.anisotropyEnable = createInfo.MaxAnisotropy > 1;
        samplerCI.maxAnisotropy = createInfo.MaxAnisotropy;
        samplerCI.compareEnable = createInfo.CompareOp != RHICompareOp::Never;
        samplerCI.compareOp = VKEnumCast<RHICompareOp, vk::CompareOp>(createInfo.CompareOp);
        samplerCI.minLod = createInfo.LodMinClamp;
        samplerCI.maxLod = createInfo.LodMaxClamp;

        mSampler = mDevice.GetVkDevice().createSampler(samplerCI);

        if (!createInfo.Name.empty())
        {
            mDevice.SetObjectName(vk::ObjectType::eSampler, reinterpret_cast<uint64_t>(VkSampler(mSampler)), createInfo.Name.c_str());
        }
    }
}