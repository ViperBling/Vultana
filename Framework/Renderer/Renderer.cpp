#include "Renderer.hpp"
#include "RHI/RHICommon.hpp"
#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>

namespace Vultana
{
    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    const bool enableValidationLayers = true;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    }; 

    static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) 
    {
        std::cerr << pCallbackData->pMessage << std::endl;
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            #if defined(WIN32)
            __debugbreak();
            #endif
        }
        return VK_FALSE;
    }

    RendererBase::~RendererBase()
    {
        CleanupSwapchain();

        mDevice.destroyPipeline(mGraphicsPipeline);
        mDevice.destroyPipelineLayout(mPipelineLayout);
        mDevice.destroyRenderPass(mRenderPass);

        for (size_t i = 0; i < 3; i++)
        {
            mDevice.destroySemaphore(mRenderFinishedSemaphores[i]);
            mDevice.destroySemaphore(mImageAvailableSemaphores[i]);
            mDevice.destroyFence(mInFlightFences[i]);
        }

        mDevice.destroyCommandPool(mCommandPool);
        mDevice.destroy();

        mInstance.destroySurfaceKHR(mSurface);
        mInstance.destroy();
    }

    void RendererBase::Init(void* windowHandle, uint32_t width, uint32_t height)
    {
        mWndHandle = static_cast<GLFWindow*>(windowHandle);
        mWidth = width;
        mHeight = height;
        
        CreateInstance();
        PickPhysicalDevice();
        CreateLogicalDevice();
        SetupDebugMessenger();
        RecreateSwapchain();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void RendererBase::RenderFrame()
    {
    }

    void RendererBase::CreateInstance()
    {
        vk::ApplicationInfo appInfo = {};
        appInfo.pApplicationName = "Vultana";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Vultana";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        std::vector<const char*> extensions = mWndHandle->GetRequiredExtensions();
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        vk::InstanceCreateInfo instanceCI {};
        instanceCI.pApplicationInfo = &appInfo;
        instanceCI.setPEnabledExtensionNames(extensions);

        if (enableValidationLayers && !CheckValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers requested, but not available!");
        }
        else
        {
            instanceCI.enabledLayerCount = 0;
        }

        mInstance = vk::createInstance(instanceCI);
        CreateWindowSurface(mInstance, mWndHandle, mSurface);
    }

    void RendererBase::SetupDebugMessenger()
    {
        if (!enableValidationLayers) return;

        mDynamicLoader.init(mInstance, mDevice);

        vk::DebugUtilsMessengerCreateInfoEXT createInfo = {};
        createInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        createInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        createInfo.pfnUserCallback = ValidationLayerCallback;

        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(createInfo, nullptr, mDynamicLoader);
    }

    std::optional<uint32_t> DetermineQueueFamilyIndex(const vk::Instance& instance, const vk::PhysicalDevice device, const vk::SurfaceKHR& surface)
    {
        auto queueFamilyProperties = device.getQueueFamilyProperties();
        uint32_t index = 0;
        for (const auto& property : queueFamilyProperties)
        {
            if ((property.queueCount > 0) &&
                (device.getSurfaceSupportKHR(index, surface)) &&
                (CheckVulkanSupport(instance, device, index)) &&
                (property.queueFlags & vk::QueueFlagBits::eGraphics) &&
                (property.queueFlags & vk::QueueFlagBits::eCompute))
            {
                return index;
            }
            index++;
        }
        return { };
    }

    void RendererBase::PickPhysicalDevice()
    {
        auto pds = mInstance.enumeratePhysicalDevices();
        for (const auto& pd : pds)
        {
            auto props = pd.getProperties();

            if (props.apiVersion < VK_API_VERSION_1_2)
            {
                continue;
            }

            auto queueFamilyIndex = DetermineQueueFamilyIndex(mInstance, pd, mSurface);
            if (!queueFamilyIndex.has_value())
            {
                continue;
            }
            mPhysicalDevice = pd;
            mQueueFamilyIndex = queueFamilyIndex.value();
            if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                break;
            }
        }
        if (!mPhysicalDevice)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    void RendererBase::CreateLogicalDevice()
    {
        auto surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
        auto presentModes = mPhysicalDevice.getSurfacePresentModesKHR(mSurface);
        auto surfaceFormats = mPhysicalDevice.getSurfaceFormatsKHR(mSurface);

        mPresentMode = vk::PresentModeKHR::eImmediate;
        if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end())
        {
            mPresentMode = vk::PresentModeKHR::eMailbox;
        }
        mPresentImageCount = std::max(surfaceCapabilities.maxImageCount, 1u);

        mSurfaceFormat = surfaceFormats.front();
        for (const auto& fmt : surfaceFormats)
        {
            if (fmt.format == vk::Format::eR8G8B8A8Unorm || fmt.format == vk::Format::eB8G8R8A8Unorm)
                mSurfaceFormat = fmt;
        }

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
        std::array queuePriorities = { 1.0f };
        deviceQueueCreateInfo.setQueueFamilyIndex(mQueueFamilyIndex);
        deviceQueueCreateInfo.setQueuePriorities(queuePriorities);

        std::vector<const char*> deviceExtensions;
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);

        vk::DeviceCreateInfo deviceCI {};
        deviceCI.setQueueCreateInfos(deviceQueueCreateInfo);
        deviceCI.setPEnabledExtensionNames(deviceExtensions);

        mDevice = mPhysicalDevice.createDevice(deviceCI);
        mGraphicsQueue = mDevice.getQueue(mQueueFamilyIndex, 0);
        mPresentQueue = mDevice.getQueue(mQueueFamilyIndex, 0);
    }

    void RendererBase::RecreateSwapchain()
    {
        mDevice.waitIdle();

        auto surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
        mSurfaceExtent = vk::Extent2D(
            std::clamp((uint32_t)mWidth, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            std::clamp((uint32_t)mHeight, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height));
        
        vk::SwapchainCreateInfoKHR swapchainCI {};
        swapchainCI.setSurface(mSurface);
        swapchainCI.setMinImageCount(mPresentImageCount);
        swapchainCI.setImageFormat(mSurfaceFormat.format);
        swapchainCI.setImageColorSpace(mSurfaceFormat.colorSpace);
        swapchainCI.setImageExtent(mSurfaceExtent);
        swapchainCI.setImageArrayLayers(1);
        swapchainCI.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapchainCI.setImageSharingMode(vk::SharingMode::eExclusive);
        swapchainCI.setPreTransform(surfaceCapabilities.currentTransform);
        swapchainCI.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapchainCI.setPresentMode(mPresentMode);
        swapchainCI.setClipped(true);
        swapchainCI.setOldSwapchain(mSwapchain);

        mSwapchain = mDevice.createSwapchainKHR(swapchainCI);

        if (swapchainCI.oldSwapchain)
        {
            mDevice.destroySwapchainKHR(swapchainCI.oldSwapchain);
        }

        auto swapchainImage = mDevice.getSwapchainImagesKHR(mSwapchain);
        mPresentImageCount = swapchainImage.size();
        mSwapchainImages.clear();
        mSwapchainImages.reserve(mPresentImageCount);
        mSwapchainImageViews.resize(mPresentImageCount);
        mSwapchainFramebuffers.resize(mPresentImageCount);

        for (size_t i = 0; i < mPresentImageCount; i++)
        {
            mSwapchainImages.push_back(swapchainImage[i]);

            vk::ImageViewCreateInfo imageViewCI {};
            imageViewCI.setImage(mSwapchainImages[i]);
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            imageViewCI.setFormat(mSurfaceFormat.format);
            imageViewCI.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            imageViewCI.setComponents(vk::ComponentMapping(
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity));
            mSwapchainImageViews[i] = mDevice.createImageView(imageViewCI);

            vk::ImageView attach[] = { mSwapchainImageViews[i] };
            vk::FramebufferCreateInfo framebufferCI {};
            framebufferCI.setRenderPass(mRenderPass);
            framebufferCI.setAttachments(attach);
            framebufferCI.setWidth(mSurfaceExtent.width);
            framebufferCI.setHeight(mSurfaceExtent.height);
            framebufferCI.setLayers(1);
            mSwapchainFramebuffers[i] = mDevice.createFramebuffer(framebufferCI);
        }
        
        mImageAvailableSemaphores.resize(RHI_MAX_IN_FLIGHT_FRAMES);
        mRenderFinishedSemaphores.resize(RHI_MAX_IN_FLIGHT_FRAMES);
        mInFlightFences.resize(RHI_MAX_IN_FLIGHT_FRAMES);

        vk::SemaphoreCreateInfo semaphoreCI {};
        vk::FenceCreateInfo fenceCI {};
        fenceCI.setFlags(vk::FenceCreateFlagBits::eSignaled);

        for (size_t i = 0; i < RHI_MAX_IN_FLIGHT_FRAMES; i++)
        {
            mImageAvailableSemaphores[i] = mDevice.createSemaphore(semaphoreCI);
            mRenderFinishedSemaphores[i] = mDevice.createSemaphore(semaphoreCI);
            mInFlightFences[i] = mDevice.createFence(fenceCI);
        }

        vk::CommandPoolCreateInfo commandPoolCI {};
        commandPoolCI.setQueueFamilyIndex(mQueueFamilyIndex);
        commandPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);

        mCommandPool = mDevice.createCommandPool(commandPoolCI);

        mCommandBuffers.resize(RHI_MAX_IN_FLIGHT_FRAMES);
        vk::CommandBufferAllocateInfo commandBufferAI {};
        commandBufferAI.setCommandPool(mCommandPool);
        commandBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
        commandBufferAI.setCommandBufferCount(mCommandBuffers.size());

        mCommandBuffers = mDevice.allocateCommandBuffers(commandBufferAI);
    }

    void RendererBase::CreateRenderPass()
    {
        vk::AttachmentDescription colorAttachment {};
        colorAttachment.format = mSurfaceFormat.format;
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colorRef {};
        colorRef.attachment = 0;
        colorRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass {};
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        vk::SubpassDependency dependency {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

        vk::RenderPassCreateInfo renderPassCI {};
        renderPassCI.setAttachments(colorAttachment);
        renderPassCI.setSubpasses(subpass);
        renderPassCI.setDependencies(dependency);

        mRenderPass = mDevice.createRenderPass(renderPassCI);
    }

    void RendererBase::CreateGraphicsPipeline()
    {

    }

    void RendererBase::CreateFramebuffers()
    {
    }

    void RendererBase::CreateCommandPool()
    {
    }

    void RendererBase::CreateCommandBuffers()
    {
    }

    void RendererBase::CreateSyncObjects()
    {
    }

    void RendererBase::CleanupSwapchain()
    {
    }

    bool RendererBase::CheckValidationLayerSupport()
    {
        auto availableLayers = vk::enumerateInstanceLayerProperties();

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }
        return true;
    }

} // namespace Vultana::Renderer
