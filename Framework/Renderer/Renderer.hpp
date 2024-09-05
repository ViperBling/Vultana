#pragma once

#include "RHI/RHI.hpp"
#include "Utilities/Utility.hpp"
#include "Utilities/Math.hpp"

#include <iostream>
#include <deque>
#include <memory>
#include <functional>

namespace Window
{
    class GLFWindow;
}

namespace Renderer
{
    class RendererBase
    {
    public:
        RendererBase();
        ~RendererBase();

        bool CreateDevice(RHI::ERHIRenderBackend backend, void* windowHandle, uint32_t width, uint32_t height);
        void RenderFrame();
        void WaitGPU();

        uint64_t GetFrameID() const { return mpDevice->GetFrameID(); }
        uint32_t GetDisplayWidth() const { return mDisplayWidth; }
        uint32_t GetDisplayHeight() const { return mDisplayHeight; }
        uint32_t GetRenderWidth() const { return mRenderWidth; }
        uint32_t GetRenderHeight() const { return mRenderHeight; }

        RHI::RHIDevice* GetDevice() const { return mpDevice.get(); }
        RHI::RHISwapchain* GetSwapchain() const { return mpSwapchain.get(); }

    private:
        void OnWindowResize(Window::GLFWindow* wndHandle, uint32_t width, uint32_t height);

        void BeginFrame();
        void UploadResource();
        void Render();
        void EndFrame();
        
    private:
        std::unique_ptr<RHI::RHIDevice> mpDevice;
        std::unique_ptr<RHI::RHISwapchain> mpSwapchain;

        uint32_t mDisplayWidth;
        uint32_t mDisplayHeight;
        uint32_t mRenderWidth;
        uint32_t mRenderHeight;
        float mUpscaleRatio = 1.0f;
        float mMipBias = 0.0f;

        uint64_t mCurrentFrameFenceValue = 0;
        uint64_t mFrameFenceValue[RHI::RHI_MAX_INFLIGHT_FRAMES] = {};
        std::unique_ptr<RHI::RHIFence> mpFrameFence;
        std::unique_ptr<RHI::RHICommandList> mpCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t mCurrentAsyncComputeFenceValue = 0;
        std::unique_ptr<RHI::RHIFence> mpAsyncComputeFence;
        std::unique_ptr<RHI::RHICommandList> mpAsyncComputeCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t mCurrentUploadFenceValue = 0;
        std::unique_ptr<RHI::RHIFence> mpUploadFence;
        std::unique_ptr<RHI::RHICommandList> mpUploadCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];


    };
}