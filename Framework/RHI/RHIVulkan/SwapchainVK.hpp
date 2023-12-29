#pragma once

#include "RHI/RHISwapchain.hpp"

#include <vulkan/vulkan.hpp>
#include <vector>

namespace Vultana
{
    class DeviceVK;
    class QueueVK;

    class SwapchainVK : public RHISwapchain
    {
    public:
        NOCOPY(SwapchainVK)
        SwapchainVK(DeviceVK& device, const SwapchainCreateInfo& createInfo);
        ~SwapchainVK();

        RHITexture* GetTexture(uint8_t index) override { return mTextures[index]; }
        uint8_t AcquireBackTexture() override;
        void Present() override;
        void Resize(const Vector2& extent) override;
        void Destroy() override;

        vk::Semaphore GetImageSemaphore() const { return mImageAvaliableSemaphore; }
        void AddWaitSemaphore(vk::Semaphore semaphore) { mWaitSemaphores.emplace_back(semaphore); }
    
    private:
        void CreateNativeSwapchain(const SwapchainCreateInfo& createInfo);

    private:
        DeviceVK& mDevice;
        vk::SwapchainKHR mSwapchain = VK_NULL_HANDLE;
        std::vector<RHITexture*> mTextures;
        vk::Queue mQueue = VK_NULL_HANDLE;
        vk::Semaphore mImageAvaliableSemaphore;
        std::vector<vk::Semaphore> mWaitSemaphores;
        Vector2 mExtent;

        uint32_t mSwapchainImageCount = 0;
        uint32_t mSwapchainImageIndex = 0;
    };
}