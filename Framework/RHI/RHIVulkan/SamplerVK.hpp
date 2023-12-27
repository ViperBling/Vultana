#pragma once

#include "RHI/RHISampler.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class DeviceVK;

    class SamplerVK : public RHISampler
    {
    public:
        NOCOPY(SamplerVK);
        SamplerVK(DeviceVK& device, const SamplerCreateInfo& createInfo);
        ~SamplerVK();
        void Destroy() override;

        vk::Sampler GetVkSampler() const { return mSampler; }

    private:
        void CreateSampler(const SamplerCreateInfo& createInfo);

    private:
        DeviceVK& mDevice;
        vk::Sampler mSampler;
    };
}