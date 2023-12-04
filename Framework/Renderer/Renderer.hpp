#pragma once

#include "RHI/RHICommon.hpp"
#include "RHI/RHIDevice.hpp"

#include <iostream>
#include <vulkan/vulkan.hpp>

namespace Vultana::Renderer
{
    class RendererBase
    {
    public:
        RendererBase();
        virtual ~RendererBase();

        void CreateDevice(void* wndHandle, uint32_t wndWidth, uint32_t wndHeight);
        void RenderFrame();

    private:
        std::unique_ptr<RHI::RHIDevice> mpDevice;
        // std::unique_ptr<vk::SwapchainKHR> mpSwapchain;

        uint32_t mDisplayWidth;
        uint32_t mDisplayHeight;
    };
}