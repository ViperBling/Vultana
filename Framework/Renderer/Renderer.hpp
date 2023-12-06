#pragma once

#include "RHI/RHICommon.hpp"
#include "RHI/RHIDevice.hpp"

#include <iostream>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class GLFWindow;
    
    class RendererBase
    {
    public:
        RendererBase() = default;
        virtual ~RendererBase();

        void Init(void* windowHandle);
        void RenderFrame();

        VkInstance GetInstance() const { return mInstance; }
        VkSurfaceKHR GetSurface() const { return mSurface; }
        VkPhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

    private:
        void CreateInstance();
        void SetupDebugMessenger();
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
        void RecreateSwapchain();

        bool CheckValidationLayerSupport();

    private:
        GLFWindow* mWndHandle;
        vk::Instance mInstance;
        vk::PhysicalDevice mPhysicalDevice;
        vk::Device mDevice;
        vk::Queue mGraphicsQueue;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        vk::DispatchLoaderDynamic mDynamicLoader;

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

        uint32_t mQueueFamilyIndex;

        size_t mCurrentFrame = 0;
        bool mFramebufferResized = false;
    };
}