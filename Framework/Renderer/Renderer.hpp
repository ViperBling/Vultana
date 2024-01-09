#pragma once

#include "RHI/RHI.hpp"
#include "Utilities/Utility.hpp"
#include "Utilities/Math.hpp"

#include <iostream>
#include <deque>
#include <memory>
#include <functional>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Window
{
    class GLFWindow;
}

namespace Renderer
{
    struct RendererCreateInfo
    {
        // RHI::RHIDeviceType DeviceType = RHI::RHIDeviceType::Hardware;
        const char* ApplicationName = "Vultana";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool bEnableValidationLayers = true;
    };
    
    class RendererBase
    {
    public:
        NOCOPY(RendererBase);
        RendererBase(Window::GLFWindow* window) : mWndHandle(window) {}
        virtual ~RendererBase();

        virtual void Init(RendererCreateInfo& createInfo);
        virtual void Cleanup();
        virtual void RenderFrame();

    private:
        void InitContext(RendererCreateInfo& createInfo);
        void InitSwapchain(RendererCreateInfo& createInfo);
        void CreateSwapchainImageView();
        void InitPipelines();
        void CreateVertexBuffer();
        void InitSyncStructures();
        void InitCommands();

        void RecordCommandBuffer();
        void SubmitCommandBuffer();

    private:
        std::unique_ptr<RHI::RHIDevice> mDevice;
        std::unique_ptr<RHI::RHISwapchain> mSwapchain;

        Math::Vector2u mSwapchainExtent;
        
        Window::GLFWindow* mWndHandle;
    };
}