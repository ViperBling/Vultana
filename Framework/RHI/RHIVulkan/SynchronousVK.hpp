#pragma once

#include "RHI/RHISynchronous.hpp"

#include <vulkan/vulkan.hpp>

namespace RHI
{
    class DeviceVK;

    class FenceVK : public RHIFence 
    {
    public:
        explicit FenceVK(DeviceVK& device);
        ~FenceVK();

        FenceStatus GetFenceStatus() override;
        void Reset() override;
        void Wait() override;
        void Destroy() override;

        vk::Fence GetVkFence() const { return mFence; }

    private:
        void CreateFence();

    private:
        DeviceVK& mDevice;
        vk::Fence mFence;
        bool mbSignaled;
    };
}