#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHISwapchain.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHISwapchainVK : public RHISwapchain
    {
    public:
        RHISwapchainVK(RHIDeviceVK* device, const RHISwapchainDesc& desc, const std::string& name);
        ~RHISwapchainVK();

        bool Create();
        void Present(vk::Queue queue);
        vk::Semaphore GetAcquireSemaphore();
        vk::Semaphore GetPresentSemaphore();

        virtual void* GetNativeHandle() const override { return mSwapchain; }
        virtual void AcquireNextBackBuffer() override;
        virtual RHITexture* GetBackBuffer() const override;
        virtual bool Resize(uint32_t width, uint32_t height) override;
        virtual void SetVSyncEnabled(bool enabled) override;

    private:
        bool CreateSurface();
        bool CreateSwapchain();
        bool CreateTextures();
        bool CreateSemaphores();

        bool RecreateSwapchain();
    
    private:
        vk::SwapchainKHR mSwapchain;
        vk::SurfaceKHR mSurface;

        bool mbEnableVSync = false;

        uint32_t mCurrentBackBuffer = 0;
        std::vector<RHITexture*> mBackBuffers;

        int32_t mFrameSemaphoreIndex = -1;
        std::vector<vk::Semaphore> mAcquireSemaphores;
        std::vector<vk::Semaphore> mPresentSemaphores;
    };
}