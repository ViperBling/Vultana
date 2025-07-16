#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHISwapchain.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHISwapchainVK : public RHISwapchain
    {
    public:
        RHISwapchainVK(RHIDeviceVK* device, const RHISwapchainDesc& desc, const eastl::string& name);
        ~RHISwapchainVK();

        bool Create();
        void Present(vk::Queue queue);
        vk::Semaphore GetAcquireSemaphore();
        vk::Semaphore GetPresentSemaphore();

        virtual void* GetNativeHandle() const override { return m_Swapchain; }
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
        vk::SwapchainKHR m_Swapchain;
        vk::SurfaceKHR m_Surface;

        bool m_bEnableVSync = false;
        bool m_bMailboxSupported = false;

        uint32_t m_CurrentBackBuffer = 0;
        eastl::vector<RHITexture*> m_BackBuffers;

        int32_t m_FrameSemaphoreIndex = -1;
        eastl::vector<vk::Semaphore> m_AcquireSemaphores;
        eastl::vector<vk::Semaphore> m_PresentSemaphores;
    };
}