#pragma once

#include "RHI/RHICommon.hpp"
#include "RHI/RHIDevice.hpp"

#include <iostream>
#include <vulkan/vulkan.hpp>

struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T*;

namespace Vultana
{
    inline void DefaultCallback(const std::string&) {}
    inline void InfoCallbackFunc(const std::string& message)
    {
        std::cout << "[INFO Renderer] : " << message << std::endl;
    }
    inline void ErrorCallbackFunc(const std::string& message)
    {
        std::cout << "[ERROR Renderer] : " << message << std::endl;
    }

    struct RendererCreateInfo
    {
        RHIDeviceType DeviceType = RHIDeviceType::DISCRETE_GPU;
        std::function<void(const std::string&)> ErrorCallback = ErrorCallbackFunc;
        std::function<void(const std::string&)> InfoCallback = InfoCallbackFunc;
        std::vector<const char*> Extensions;
        std::vector<const char*> ValidationLayers;
        const char* ApplicationName = "Vultana";
    };

    class GLFWindow;
    
    class RendererBase
    {
    public:
        RendererBase(const RendererCreateInfo& createInfo);
        virtual ~RendererBase();

        void Init(const vk::SurfaceKHR& surface, RendererCreateInfo& createInfo);
        void RecreateSwapchian(uint32_t width, uint32_t height);
        void RenderFrame();

        VkInstance GetInstance() const { return mInstance; }
        VkSurfaceKHR GetSurface() const { return mSurface; }
        VkPhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

    private:
        vk::Instance mInstance;
        vk::SurfaceKHR mSurface;
        vk::SurfaceFormatKHR mSurfaceFormat;
        vk::PresentModeKHR mPresentMode;
        vk::Extent2D mSurfaceExtent;
        uint32_t mPresentImageCount = 0;

        vk::PhysicalDevice mPhysicalDevice;
        vk::PhysicalDeviceProperties mPDProps;

        vk::Device mDevice;
        vk::Queue mQueue;
        vk::Semaphore mImageAvailableSemaphore;
        vk::Semaphore mRenderFinishedSemaphore;
        vk::Fence mImmdiateFence;
        vk::CommandPool mCommandPool;
        vk::CommandBuffer mCommandBuffer;

        vk::SwapchainKHR mSwapchain;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        vk::DispatchLoaderDynamic mDynamicLoader;

        VmaAllocator mAllocator;
        
        std::vector<vk::Image> mSwapchainImages;
        std::vector<vk::ImageView> mSwapchainImageViews;

        vk::RenderPass mRenderPass;
        vk::PipelineLayout mPipelineLayout;
        vk::Pipeline mGraphicsPipeline;

        std::vector<vk::Framebuffer> mFramebuffers;

        uint32_t mQueueFamilyIndex;

        size_t mCurrentFrame = 0;
        bool mFramebufferResized = false;
    };
}