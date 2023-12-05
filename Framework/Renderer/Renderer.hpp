#pragma once

#include "RHI/RHICommon.hpp"
#include "RHI/RHIDevice.hpp"

#include <iostream>
#include <vulkan/vulkan.hpp>

namespace Vultana::Renderer
{
    class RendererBase
    {
        friend class Window;

    public:
        RendererBase() = default;
        virtual ~RendererBase();

        void Init();
        void ConnectSurface(void* windowHandle);
        void RenderFrame();

        VkInstance GetInstance() const { return mInstance; }
        VkSurfaceKHR GetSurface() const { return mSurface; }

    private:
        void InitVulkan();
        void CreateInstance();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapchain();
        void CreateImageViews();
        void CreateRenderPass();
        void CreateGraphicsPipeline();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void CleanupSwapchain();

        bool CheckValidationLayerSupport();

    private:
        vk::Instance mInstance;
        vk::PhysicalDevice mPhysicalDevice;
        vk::Device mDevice;
        vk::Queue mGraphicsQueue;

        vk::SurfaceKHR mSurface;
        vk::SwapchainKHR mSwapchain;
        vk::Format mSwapchainImageFormat;
        vk::Extent2D mSwapchainExtent;
        std::vector<vk::Image> mSwapchainImages;
        std::vector<vk::ImageView> mSwapchainImageViews;
        vk::Queue mPresentQueue;

        vk::RenderPass mRenderPass;
        vk::PipelineLayout mPipelineLayout;
        vk::Pipeline mGraphicsPipeline;

        std::vector<vk::Framebuffer> mSwapchainFramebuffers;
        vk::CommandPool mCommandPool;

        std::vector<vk::CommandBuffer> mCommandBuffers;
        std::vector<vk::Semaphore> mImageAvailableSemaphores;
        std::vector<vk::Semaphore> mRenderFinishedSemaphores;
        std::vector<vk::Fence> mInFlightFences;

        size_t mCurrentFrame = 0;
        bool mFramebufferResized = false;
    };
}