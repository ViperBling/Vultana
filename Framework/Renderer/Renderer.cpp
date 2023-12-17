#include "Renderer.hpp"
#include "RHI/RHICommon.hpp"
#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

#include <vk_mem_alloc.h>
#include <vkbootstrap/VkBootstrap.h>

namespace Vultana
{
    const char* ValidationLayerName = "VK_LAYER_KHRONOS_validation";

    std::optional<uint32_t> DetermineQueueFamilyIndex(const vk::Instance& instance, const vk::PhysicalDevice device, const vk::SurfaceKHR& surface)
    {
        auto queueFamilyProperties = device.getQueueFamilyProperties();
        uint32_t index = 0;
        for (const auto& property : queueFamilyProperties)
        {
            if ((property.queueCount > 0) &&
                (device.getSurfaceSupportKHR(index, surface)) &&
                (CheckVulkanPresentationSupport(instance, device, index)) &&
                (property.queueFlags & vk::QueueFlagBits::eGraphics) &&
                (property.queueFlags & vk::QueueFlagBits::eCompute))
            {
                return index;
            }
            index++;
        }
        return { };
    }

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
        Cleanup();
    }

    void RendererBase::Init(RendererCreateInfo& createInfo)
    {
        InitVulkan(createInfo);
        InitSwapchain(createInfo);
        InitCommands();
        InitSyncStructures();

        mbInitialized = true;
    }

    void RendererBase::Cleanup()
    {
        if (!mbInitialized) return;

        mDevice.waitIdle();
        mDevice.destroyCommandPool(mCommandPool);

        mDevice.destroyFence(mImmdiateFence);
        mDevice.destroySemaphore(mImageAvailableSemaphore);
        mDevice.destroySemaphore(mRenderFinishedSemaphore);

        mDevice.destroySwapchainKHR(mSwapchain);

        for (auto& imageView : mSwapchainImageViews)
        {
            mDevice.destroyImageView(imageView);
        }

        mInstance.destroySurfaceKHR(mSurface);
        mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger);
        mInstance.destroy();
    }

    void RendererBase::RenderFrame()
    {
        mDevice.waitForFences(mImmdiateFence, true, UINT64_MAX);
        mDevice.resetFences(mImmdiateFence);

        mCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

        uint32_t swapchainImageIndex;
        mDevice.acquireNextImageKHR(mSwapchain, UINT64_MAX, mImageAvailableSemaphore, nullptr, &swapchainImageIndex);

        vk::CommandBufferBeginInfo cmdBufferBI {};
        cmdBufferBI.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        mCommandBuffer.begin(cmdBufferBI);

        vk::ClearColorValue clearValue;
        float flash = abs(sin(mFrameIndex / 120.0f));
        clearValue.setFloat32({0.0f, 0.0f, flash, 1.0f});

        TransitionImage(mCommandBuffer, mSwapchainImages[swapchainImageIndex], ImageTransitionMode::ToGeneral);

        vk::ImageSubresourceRange subresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        mCommandBuffer.clearColorImage(mSwapchainImages[swapchainImageIndex], vk::ImageLayout::eGeneral, clearValue, subresourceRange);

        TransitionImage(mCommandBuffer, mSwapchainImages[swapchainImageIndex], ImageTransitionMode::GeneralToPresent);

        mCommandBuffer.end();

        vk::CommandBufferSubmitInfo cmdBufferSI {};
        cmdBufferSI.setCommandBuffer(mCommandBuffer);

        vk::SemaphoreSubmitInfo waitInfo {};
        waitInfo.setSemaphore(mImageAvailableSemaphore);
        waitInfo.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
        vk::SemaphoreSubmitInfo signalInfo {};
        signalInfo.setSemaphore(mRenderFinishedSemaphore);
        signalInfo.setStageMask(vk::PipelineStageFlagBits2::eAllGraphics);

        vk::SubmitInfo2 submitInfo {};
        submitInfo.setCommandBufferInfos(cmdBufferSI);
        submitInfo.setWaitSemaphoreInfos(waitInfo);
        submitInfo.setSignalSemaphoreInfos(signalInfo);

        mQueue.submit2KHR(submitInfo, mImmdiateFence);

        vk::PresentInfoKHR presentInfo {};
        presentInfo.setSwapchains(mSwapchain);
        presentInfo.setWaitSemaphores(mRenderFinishedSemaphore);
        presentInfo.setImageIndices(swapchainImageIndex);

        mQueue.presentKHR(presentInfo);

        mFrameIndex++;
    }

    void RendererBase::InitVulkan(RendererCreateInfo& createInfo)
    {
        // vk::ApplicationInfo appInfo {};
        // appInfo.setPApplicationName(createInfo.ApplicationName);
        // appInfo.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
        // appInfo.setPEngineName(createInfo.ApplicationName);
        // appInfo.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
        // appInfo.setApiVersion(VK_API_VERSION_1_3);
        
        // auto supportedExt = vk::enumerateInstanceExtensionProperties();
        // auto supportedLayers = vk::enumerateInstanceLayerProperties();

        // auto extensions = mWndHandle->GetRequiredExtensions();
        // extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        // createInfo.InfoCallback("Enumerating request extensions: ");
        // for (auto& ext : extensions)
        // {
        //     createInfo.InfoCallback("- " + std::string(ext));

        //     auto extIt = std::find_if(supportedExt.begin(), supportedExt.end(), 
        //     [ext](const vk::ExtensionProperties& extension)
        //         {
        //             return std::strcmp(extension.extensionName.data(), ext) == 0;
        //         });
        //     if (extIt == supportedExt.end())
        //     {
        //         createInfo.ErrorCallback("Extension " + std::string(ext) + " is not supported");
        //         break;
        //     }
        // }

        // std::vector<const char*> layers;
        // if (createInfo.bEnableValidationLayers) layers.push_back(ValidationLayerName);

        // createInfo.InfoCallback("Enumerating request layers: ");
        // for (auto& layer : layers)
        // {
        //     createInfo.InfoCallback("- " + std::string(layer));
        //     auto layerIt = std::find_if(supportedLayers.begin(), supportedLayers.end(), 
        //     [layer](const vk::LayerProperties& properties)
        //         {
        //             return std::strcmp(properties.layerName.data(), layer) == 0;
        //         });
        //     if (layerIt == supportedLayers.end())
        //     {
        //         createInfo.ErrorCallback("Layer " + std::string(layer) + " is not supported");
        //         break;
        //     }
        // }
        
        // vk::InstanceCreateInfo instanceCI {};
        // instanceCI.setPApplicationInfo(&appInfo);
        // instanceCI.setPEnabledExtensionNames(extensions);
        // instanceCI.setPEnabledLayerNames(layers);

        // mInstance = vk::createInstance(instanceCI);
        // createInfo.InfoCallback("Created vulkan instance");
        // mSurface = mWndHandle->CreateWindowSurface(*this);
        // createInfo.InfoCallback("Created surface");
        
        // createInfo.InfoCallback("Enumerating physical devices: ");
        // auto physicalDevices = mInstance.enumeratePhysicalDevices();
        // for (auto& pd : physicalDevices)
        // {
        //     auto properties = pd.getProperties();
        //     createInfo.InfoCallback("- " + std::string(properties.deviceName.data()));

        //     auto queueFamilyIndex = DetermineQueueFamilyIndex(mInstance, pd, mSurface);
        //     if (queueFamilyIndex.has_value())
        //     {
        //         createInfo.InfoCallback("Found suitable physical device" + std::string(properties.deviceName.data()));
        //         mPhysicalDevice = pd;
        //         mQueueFamilyIndex = queueFamilyIndex.value();
        //         break;
        //     }
        // }

        // auto presentModes = mPhysicalDevice.getSurfacePresentModesKHR(mSurface);
        // auto surfaceCaps = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
        // auto surfaceFormats = mPhysicalDevice.getSurfaceFormatsKHR(mSurface);

        // mPresentMode = vk::PresentModeKHR::eImmediate;
        // if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end()) { mPresentMode = vk::PresentModeKHR::eMailbox; }
        
        // mSurfaceFormat = surfaceFormats.front();
        // for (const auto& fmt : surfaceFormats)
        // {
        //     if (fmt.format == vk::Format::eR8G8B8A8Unorm || fmt.format == vk::Format::eB8G8R8A8Unorm)
        //     {
        //         mSurfaceFormat = fmt;
        //         break;
        //     }
        // }

        // createInfo.InfoCallback("Selected surface format: " + std::string(vk::to_string(mSurfaceFormat.format)));
        // createInfo.InfoCallback("Selected present mode: " + std::string(vk::to_string(mPresentMode)));

        // vk::DeviceQueueCreateInfo deviceQueueCI {};
        // std::array queuePriorities = {1.0f};
        // deviceQueueCI.setQueueFamilyIndex(mQueueFamilyIndex);
        // deviceQueueCI.setQueuePriorities(queuePriorities);

        // std::vector<const char*> deviceExtensions;
        // deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // vk::DeviceCreateInfo deviceCI {};
        // deviceCI.setQueueCreateInfos(deviceQueueCI);
        // deviceCI.setPEnabledExtensionNames(deviceExtensions);

        // mDevice = mPhysicalDevice.createDevice(deviceCI);
        // mQueue = mDevice.getQueue(mQueueFamilyIndex, 0);

        // createInfo.InfoCallback("Created logical device and queue");

        // mDynamicLoader.init(mInstance, mDevice);

        // vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCI {};
        // debugMessengerCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        // debugMessengerCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        // debugMessengerCI.setPfnUserCallback(ValidationLayerCallback);
        // mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugMessengerCI, nullptr, mDynamicLoader);

        // VmaAllocatorCreateInfo allocatorCI {};
        // allocatorCI.physicalDevice = mPhysicalDevice;
        // allocatorCI.device = mDevice;
        // allocatorCI.instance = mInstance;
        // vmaCreateAllocator(&allocatorCI, &mAllocator);

        // createInfo.InfoCallback("Created vulkan memory allocator");

        vkb::InstanceBuilder instBuilder;
        auto instRet = instBuilder.set_app_name(createInfo.ApplicationName)
            .request_validation_layers(createInfo.bEnableValidationLayers)
            .require_api_version(1, 3, 0)
            .use_default_debug_messenger()
            .build();
        
        vkb::Instance vkbInst = instRet.value();
        mInstance = vkbInst.instance;
        mDebugMessenger = vkbInst.debug_messenger;

        mSurface = mWndHandle->CreateWindowSurface(*this);

        vk::PhysicalDeviceVulkan13Features features{};
        features.dynamicRendering = true;
        features.synchronization2 = true;

        vkb::PhysicalDeviceSelector selector { vkbInst };
        vkb::PhysicalDevice vkbPhysicalDevice = selector
            .set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_surface(mSurface)
            .select()
            .value();
        vkb::DeviceBuilder deviceBuilder { vkbPhysicalDevice };
        vkb::Device vkbDevice = deviceBuilder.build().value();

        mDevice = vkbDevice.device;
        mPhysicalDevice = vkbPhysicalDevice.physical_device;

        mQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        mQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    }

    void RendererBase::InitSwapchain(RendererCreateInfo& createInfo)
    {
        // auto swapchainCaps = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
        // mSurfaceExtent = vk::Extent2D(
        //     std::clamp(createInfo.Width, swapchainCaps.minImageExtent.width, swapchainCaps.maxImageExtent.width),
        //     std::clamp(createInfo.Height, swapchainCaps.minImageExtent.height, swapchainCaps.maxImageExtent.height)
        // );

        // vk::SwapchainCreateInfoKHR swapchainCI {};
        // swapchainCI.setSurface(mSurface);
        // swapchainCI.setMinImageCount(swapchainCaps.minImageCount);
        // swapchainCI.setMinImageCount(swapchainCaps.minImageCount);
        // swapchainCI.setImageFormat(mSurfaceFormat.format);
        // swapchainCI.setImageColorSpace(mSurfaceFormat.colorSpace);
        // swapchainCI.setImageExtent(mSurfaceExtent);
        // swapchainCI.setImageArrayLayers(1);
        // swapchainCI.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        // swapchainCI.setImageSharingMode(vk::SharingMode::eExclusive);
        // swapchainCI.setPreTransform(swapchainCaps.currentTransform);
        // swapchainCI.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        // swapchainCI.setPresentMode(mPresentMode);
        // swapchainCI.setClipped(true);
        // swapchainCI.setOldSwapchain(mSwapchain);

        // mSwapchain = mDevice.createSwapchainKHR(swapchainCI);
        // mSwapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);
        // for (uint32_t i = 0; i < mSwapchainImages.size(); i++)
        // {
        //     vk::ImageViewCreateInfo imageViewCI {};
        //     imageViewCI.setImage(mSwapchainImages[i]);
        //     imageViewCI.setViewType(vk::ImageViewType::e2D);
        //     imageViewCI.setFormat(mSurfaceFormat.format);
        //     imageViewCI.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        //     mSwapchainImageViews.push_back(mDevice.createImageView(imageViewCI));
        // }

        vkb::SwapchainBuilder swapchainBuilder { mPhysicalDevice, mDevice, mSurface };
        vkb::Swapchain vkbSwapchain = swapchainBuilder
            .set_desired_format({ static_cast<VkFormat>(vk::Format::eR8G8B8A8Srgb), static_cast<VkColorSpaceKHR>(vk::ColorSpaceKHR::eSrgbNonlinear) })
            .set_desired_present_mode(static_cast<VkPresentModeKHR>(mPresentMode))
            .set_desired_extent(createInfo.Width, createInfo.Height)
            .set_image_usage_flags(static_cast<VkImageUsageFlags>(vk::ImageUsageFlagBits::eTransferDst))
            .build()
            .value();
        
        mSwapchain = vkbSwapchain.swapchain;
        for (auto& image : vkbSwapchain.get_images().value())
        {
            mSwapchainImages.push_back(image);
        }
        for (auto& view : vkbSwapchain.get_image_views().value())
        {
            mSwapchainImageViews.push_back(view);
        }

        mSwapchainFormat = static_cast<vk::Format>(vkbSwapchain.image_format);
    }

    void RendererBase::InitRenderpass()
    {

    }

    void RendererBase::InitFramebuffers()
    {

    }

    void RendererBase::InitCommands()
    {
        vk::CommandPoolCreateInfo cmdPoolCI {};
        cmdPoolCI.setQueueFamilyIndex(mQueueFamilyIndex);
        cmdPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        mCommandPool = mDevice.createCommandPool(cmdPoolCI);

        vk::CommandBufferAllocateInfo cmdBufferAI {};
        cmdBufferAI.setCommandPool(mCommandPool);
        cmdBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
        cmdBufferAI.setCommandBufferCount(1);

        mCommandBuffer = mDevice.allocateCommandBuffers(cmdBufferAI).front();
    }

    void RendererBase::InitSyncStructures()
    {
        vk::FenceCreateInfo fenceCI {};
        fenceCI.setFlags(vk::FenceCreateFlagBits::eSignaled);
        mImmdiateFence = mDevice.createFence(fenceCI);

        vk::SemaphoreCreateInfo semaphoreCI {};
        mImageAvailableSemaphore = mDevice.createSemaphore(semaphoreCI);
        mRenderFinishedSemaphore = mDevice.createSemaphore(semaphoreCI);
    }

    void RendererBase::TransitionImage(vk::CommandBuffer cmdBuffer, vk::Image image, ImageTransitionMode transitionMode)
    {
        vk::ImageMemoryBarrier2 imageBarrier{};
        imageBarrier.setSrcStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
        imageBarrier.setSrcAccessMask(vk::AccessFlagBits2KHR::eNone);
        imageBarrier.setDstStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
        imageBarrier.setDstAccessMask(vk::AccessFlagBits2KHR::eNone);

        switch (transitionMode)
        {
        case ImageTransitionMode::ToAttachment:
            imageBarrier.oldLayout = vk::ImageLayout::eUndefined;
            imageBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
            break;
        case ImageTransitionMode::AttachmentToPresent:
            imageBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
            imageBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
            break;
        case ImageTransitionMode::ToGeneral:
            imageBarrier.oldLayout = vk::ImageLayout::eUndefined;
            imageBarrier.newLayout = vk::ImageLayout::eGeneral;
            break;
        case ImageTransitionMode::GeneralToPresent:
            imageBarrier.oldLayout = vk::ImageLayout::eGeneral;
            imageBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
            break;
        
        default:
            break;
        }

        imageBarrier.image = image;
        imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

        vk::DependencyInfo dependencyInfo{};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &imageBarrier;

        // mDynamicLoader.vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
        cmdBuffer.pipelineBarrier2KHR(dependencyInfo);
    }
} // namespace Vultana::Renderer
