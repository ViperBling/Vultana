#pragma once

#include "RHI/RHICommon.hpp"
#include "RHI/RHIDevice.hpp"
#include "Utilities/Utility.hpp"

#include <iostream>
#include <deque>
#include <functional>
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
        const char* ApplicationName = "Vultana";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool bEnableValidationLayers = true;
    };

    enum class ImageTransitionMode
    {
        ToAttachment,
        ToGeneral,
        GeneralToPresent,
        AttachmentToPresent
    };

    struct DeletionQueue
    {
        std::deque<std::function<void()>> deletors;

        void PushFunction(std::function<void()>&& function)
        {
            deletors.push_back(function);
        }

        void Flush()
        {
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            {
                (*it)();
            }
            deletors.clear();
        }
    };

    class GLFWindow;
    
    class RendererBase
    {
    public:
        NOCOPY(RendererBase);
        RendererBase(GLFWindow* window) : mWndHandle(window) {}
        virtual ~RendererBase();

        void Init(RendererCreateInfo& createInfo);
        void Cleanup();
        void RenderFrame();

        VkInstance GetInstance() const { return mInstance; }
        VkSurfaceKHR GetSurface() const { return mSurface; }
        VkPhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }
        uint32_t GetQueueFamilyIndex() const { return mQueueFamilyIndex; }

    private:
        void InitVulkan(RendererCreateInfo& createInfo);
        void InitSwapchain(RendererCreateInfo& createInfo);
        void InitRenderpass();
        void InitFramebuffers();
        void InitCommands();
        void InitSyncStructures();

        void TransitionImage(vk::CommandBuffer cmdBuffer, vk::Image image, ImageTransitionMode transitionMode);

    private:
        vk::Instance mInstance;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        vk::PhysicalDevice mPhysicalDevice;
        vk::Device mDevice;

        vk::Semaphore mImageAvailableSemaphore;
        vk::Semaphore mRenderFinishedSemaphore;
        vk::Fence mImmdiateFence;

        vk::Queue mQueue;
        uint32_t mQueueFamilyIndex;

        vk::CommandPool mCommandPool;
        vk::CommandBuffer mCommandBuffer;

        vk::RenderPass mRenderPass;

        vk::SurfaceKHR mSurface;
        vk::SwapchainKHR mSwapchain;
        vk::PresentModeKHR mPresentMode;
        vk::SurfaceFormatKHR mSurfaceFormat;
        vk::Format mSwapchainFormat;

        std::vector<vk::Image> mSwapchainImages;
        std::vector<vk::ImageView> mSwapchainImageViews;
        std::vector<vk::Framebuffer> mFramebuffers;

        vk::Extent2D mSurfaceExtent;

        vk::DispatchLoaderDynamic mDynamicLoader;

        VmaAllocator mAllocator = { };

        bool mbInitialized = false;
        uint32_t mFrameIndex = 0;

        GLFWindow* mWndHandle;
    };
}