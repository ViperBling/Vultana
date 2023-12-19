#include "Renderer.hpp"
#include "RHI/RHICommon.hpp"
#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Vultana
{
    const char *ValidationLayerName = "VK_LAYER_KHRONOS_validation";

    std::optional<uint32_t> DetermineQueueFamilyIndex(const vk::Instance &instance, const vk::PhysicalDevice device, const vk::SurfaceKHR &surface)
    {
        auto queueFamilyProperties = device.getQueueFamilyProperties();
        uint32_t index = 0;
        for (const auto &property : queueFamilyProperties)
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
        return {};
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
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

    void RendererBase::Init(RendererCreateInfo &createInfo)
    {
        InitVulkan(createInfo);
        InitSwapchain(createInfo);
        InitCommands();
        InitSyncStructures();
        InitDescriptors();
        InitPipelines();

        mbInitialized = true;
    }

    void RendererBase::Cleanup()
    {
        if (!mbInitialized)
            return;

        mDevice.waitIdle();
        mDevice.destroyCommandPool(mCommandPool);

        mDevice.destroyFence(mImmdiateFence);
        mDevice.destroySemaphore(mImageAvailableSemaphore);
        mDevice.destroySemaphore(mRenderFinishedSemaphore);

        mDevice.destroySwapchainKHR(mSwapchain);

        for (auto &imageView : mSwapchainImageViews)
        {
            mDevice.destroyImageView(imageView);
        }

        mDeletionQueue.Flush();
        mDevice.destroyPipeline(mPipeline);
        mDevice.destroyPipelineLayout(mPipelineLayout);
        mDevice.destroyDescriptorSetLayout(mDescriptorSetLayout);
        mDevice.destroyDescriptorPool(mDescriptorPool);

        mDevice.destroy();
        mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger, nullptr, mDynamicLoader);
        mInstance.destroySurfaceKHR(mSurface);
        mInstance.destroy();
    }

    void RendererBase::RenderFrame()
    {
        vk::resultCheck(mDevice.waitForFences(mImmdiateFence, true, UINT64_MAX), "WaitFences");
        mDevice.resetFences(mImmdiateFence);

        mCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

        uint32_t swapchainImageIndex;
        vk::resultCheck(mDevice.acquireNextImageKHR(mSwapchain, UINT64_MAX, mImageAvailableSemaphore, nullptr, &swapchainImageIndex), "GrabNextImage");

        vk::CommandBufferBeginInfo cmdBufferBI{};
        cmdBufferBI.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        mCommandBuffer.begin(cmdBufferBI);

        TransitionImage(mCommandBuffer, mDrawImage.Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

        mCommandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, mPipeline);
        mCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, mPipelineLayout, 0, mDescriptorSet, nullptr);
        mCommandBuffer.dispatch(mSurfaceExtent.width / 16, mSurfaceExtent.height / 16, 1);

        TransitionImage(mCommandBuffer, mDrawImage.Image, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
        TransitionImage(mCommandBuffer, mSwapchainImages[swapchainImageIndex], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        vk::ImageSubresourceRange clearRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        vk::Extent3D extent = {
            mSurfaceExtent.width,
            mSurfaceExtent.height,
            1
        };

        CopyImage(mCommandBuffer, mDrawImage.Image, mSwapchainImages[swapchainImageIndex], extent);

        TransitionImage(mCommandBuffer, mSwapchainImages[swapchainImageIndex], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);

        mCommandBuffer.end();

        vk::CommandBufferSubmitInfo cmdBufferSI{};
        cmdBufferSI.setCommandBuffer(mCommandBuffer);

        vk::SemaphoreSubmitInfo waitInfo{};
        waitInfo.setSemaphore(mImageAvailableSemaphore);
        waitInfo.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
        vk::SemaphoreSubmitInfo signalInfo{};
        signalInfo.setSemaphore(mRenderFinishedSemaphore);
        signalInfo.setStageMask(vk::PipelineStageFlagBits2::eAllGraphics);

        vk::SubmitInfo2 submitInfo{};
        submitInfo.setCommandBufferInfos(cmdBufferSI);
        submitInfo.setWaitSemaphoreInfos(waitInfo);
        submitInfo.setSignalSemaphoreInfos(signalInfo);

        mQueue.submit2(submitInfo, mImmdiateFence);

        vk::PresentInfoKHR presentInfo{};
        presentInfo.setSwapchains(mSwapchain);
        presentInfo.setWaitSemaphores(mRenderFinishedSemaphore);
        presentInfo.setImageIndices(swapchainImageIndex);

        vk::resultCheck(mQueue.presentKHR(presentInfo), "Present");

        mFrameIndex++;
    }

    void RendererBase::InitVulkan(RendererCreateInfo &createInfo)
    {
        vk::ApplicationInfo appInfo{};
        appInfo.setPApplicationName(createInfo.ApplicationName);
        appInfo.setApplicationVersion(VK_MAKE_VERSION(1, 3, 0));
        appInfo.setPEngineName(createInfo.ApplicationName);
        appInfo.setEngineVersion(VK_MAKE_VERSION(1, 3, 0));
        appInfo.setApiVersion(VK_API_VERSION_1_3);

        auto supportedExt = vk::enumerateInstanceExtensionProperties();
        auto supportedLayers = vk::enumerateInstanceLayerProperties();

        auto extensions = mWndHandle->GetRequiredExtensions();
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        createInfo.InfoCallback("Enumerating request extensions: ");
        for (auto &ext : extensions)
        {
            createInfo.InfoCallback("- " + std::string(ext));

            auto extIt = std::find_if(supportedExt.begin(), supportedExt.end(),
                                      [ext](const vk::ExtensionProperties &extension)
                                      {
                                          return std::strcmp(extension.extensionName.data(), ext) == 0;
                                      });
            if (extIt == supportedExt.end())
            {
                createInfo.ErrorCallback("Extension " + std::string(ext) + " is not supported");
                break;
            }
        }

        std::vector<const char *> layers;
        if (createInfo.bEnableValidationLayers)
            layers.push_back(ValidationLayerName);

        createInfo.InfoCallback("Enumerating request layers: ");
        for (auto &layer : layers)
        {
            createInfo.InfoCallback("- " + std::string(layer));
            auto layerIt = std::find_if(supportedLayers.begin(), supportedLayers.end(),
                                        [layer](const vk::LayerProperties &properties)
                                        {
                                            return std::strcmp(properties.layerName.data(), layer) == 0;
                                        });
            if (layerIt == supportedLayers.end())
            {
                createInfo.ErrorCallback("Layer " + std::string(layer) + " is not supported");
                break;
            }
        }

        vk::InstanceCreateInfo instanceCI{};
        instanceCI.setPApplicationInfo(&appInfo);
        instanceCI.setPEnabledExtensionNames(extensions);
        instanceCI.setPEnabledLayerNames(layers);

        mInstance = vk::createInstance(instanceCI);
        createInfo.InfoCallback("Created vulkan instance");
        mSurface = mWndHandle->CreateWindowSurface(*this);
        createInfo.InfoCallback("Created surface");

        createInfo.InfoCallback("Enumerating physical devices: ");
        auto physicalDevices = mInstance.enumeratePhysicalDevices();
        for (auto &pd : physicalDevices)
        {
            auto properties = pd.getProperties();
            createInfo.InfoCallback("- " + std::string(properties.deviceName.data()));

            auto queueFamilyIndex = DetermineQueueFamilyIndex(mInstance, pd, mSurface);
            if (queueFamilyIndex.has_value())
            {
                createInfo.InfoCallback("Found suitable physical device" + std::string(properties.deviceName.data()));
                mPhysicalDevice = pd;
                mQueueFamilyIndex = queueFamilyIndex.value();
                break;
            }
        }

        auto presentModes = mPhysicalDevice.getSurfacePresentModesKHR(mSurface);
        auto surfaceCaps = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
        auto surfaceFormats = mPhysicalDevice.getSurfaceFormatsKHR(mSurface);

        mPresentMode = vk::PresentModeKHR::eImmediate;
        if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end())
        {
            mPresentMode = vk::PresentModeKHR::eMailbox;
        }

        mSurfaceFormat = surfaceFormats.front();
        for (const auto &fmt : surfaceFormats)
        {
            if (fmt.format == vk::Format::eR8G8B8A8Unorm || fmt.format == vk::Format::eB8G8R8A8Unorm)
            {
                mSurfaceFormat = fmt;
                break;
            }
        }

        createInfo.InfoCallback("Selected surface format: " + std::string(vk::to_string(mSurfaceFormat.format)));
        createInfo.InfoCallback("Selected present mode: " + std::string(vk::to_string(mPresentMode)));

        vk::DeviceQueueCreateInfo deviceQueueCI{};
        std::array queuePriorities = {1.0f};
        deviceQueueCI.setQueueFamilyIndex(mQueueFamilyIndex);
        deviceQueueCI.setQueuePriorities(queuePriorities);

        std::vector<const char *> deviceExtensions;
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // 由于使用了PipelineBarrier2，所以必须启用Sync2Feature
        vk::PhysicalDeviceVulkan12Features features12{};
        features12.setBufferDeviceAddress(true);
        features12.setDescriptorIndexing(true);
        vk::PhysicalDeviceVulkan13Features features13{};
        features13.setSynchronization2(true);
        features13.setDynamicRendering(true);
        features12.setPNext(&features13);

        vk::DeviceCreateInfo deviceCI{};
        deviceCI.setQueueCreateInfos(deviceQueueCI);
        deviceCI.setPEnabledExtensionNames(deviceExtensions);
        deviceCI.setPNext(&features12);

        mDevice = mPhysicalDevice.createDevice(deviceCI);
        mQueue = mDevice.getQueue(mQueueFamilyIndex, 0);

        createInfo.InfoCallback("Created logical device and queue");

        mDynamicLoader.init(mInstance, vkGetInstanceProcAddr, mDevice, vkGetDeviceProcAddr);

        vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCI{};
        debugMessengerCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        debugMessengerCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugMessengerCI.setPfnUserCallback(ValidationLayerCallback);
        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugMessengerCI, nullptr, mDynamicLoader);

        VmaAllocatorCreateInfo allocatorCI{};
        allocatorCI.physicalDevice = mPhysicalDevice;
        allocatorCI.device = mDevice;
        allocatorCI.instance = mInstance;
        vmaCreateAllocator(&allocatorCI, &mAllocator);

        mDeletionQueue.PushFunction([=]() { vmaDestroyAllocator(mAllocator); });

        createInfo.InfoCallback("Created vulkan memory allocator");
    }

    void RendererBase::InitSwapchain(RendererCreateInfo &createInfo)
    {
        auto swapchainCaps = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
        mSurfaceExtent = vk::Extent2D(
            std::clamp(createInfo.Width, swapchainCaps.minImageExtent.width, swapchainCaps.maxImageExtent.width),
            std::clamp(createInfo.Height, swapchainCaps.minImageExtent.height, swapchainCaps.maxImageExtent.height));

        vk::SwapchainCreateInfoKHR swapchainCI{};
        swapchainCI.setSurface(mSurface);
        swapchainCI.setMinImageCount(swapchainCaps.minImageCount);
        swapchainCI.setMinImageCount(swapchainCaps.minImageCount);
        swapchainCI.setImageFormat(mSurfaceFormat.format);
        swapchainCI.setImageColorSpace(mSurfaceFormat.colorSpace);
        swapchainCI.setImageExtent(mSurfaceExtent);
        swapchainCI.setImageArrayLayers(1);
        swapchainCI.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        swapchainCI.setImageSharingMode(vk::SharingMode::eExclusive);
        swapchainCI.setPreTransform(swapchainCaps.currentTransform);
        swapchainCI.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapchainCI.setPresentMode(mPresentMode);
        swapchainCI.setClipped(true);
        swapchainCI.setOldSwapchain(mSwapchain);

        mSwapchain = mDevice.createSwapchainKHR(swapchainCI);
        mSwapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);
        for (uint32_t i = 0; i < mSwapchainImages.size(); i++)
        {
            vk::ImageViewCreateInfo imageViewCI{};
            imageViewCI.setImage(mSwapchainImages[i]);
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            imageViewCI.setFormat(mSurfaceFormat.format);
            imageViewCI.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            mSwapchainImageViews.push_back(mDevice.createImageView(imageViewCI));
        }

        vk::Extent3D drawImageExtent = {
            mSurfaceExtent.width,
            mSurfaceExtent.height,
            1
        };
        
        vk::ImageCreateInfo drawImageCI{};
        drawImageCI.setImageType(vk::ImageType::e2D);
        drawImageCI.setFormat(mSurfaceFormat.format);
        drawImageCI.setExtent(drawImageExtent);
        drawImageCI.setMipLevels(1);
        drawImageCI.setArrayLayers(1);
        drawImageCI.setSamples(vk::SampleCountFlagBits::e1);
        drawImageCI.setTiling(vk::ImageTiling::eOptimal);
        drawImageCI.setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc);

        VmaAllocationCreateInfo drawImageAllocCI {};
        drawImageAllocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        drawImageAllocCI.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        vmaCreateImage(mAllocator, (VkImageCreateInfo*)&drawImageCI,  &drawImageAllocCI, (VkImage*)&mDrawImage.Image, &mDrawImage.ImageAllocation, nullptr);
        
        vk::ImageViewCreateInfo drawImageViewCI{};
        drawImageViewCI.setImage(mDrawImage.Image);
        drawImageViewCI.setViewType(vk::ImageViewType::e2D);
        drawImageViewCI.setFormat(mSurfaceFormat.format);
        drawImageViewCI.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        mDrawImageView = mDevice.createImageView(drawImageViewCI);

        mDeletionQueue.PushFunction([=]()
        {
            mDevice.destroyImageView(mDrawImageView);
            vmaDestroyImage(mAllocator, mDrawImage.Image, mDrawImage.ImageAllocation);
        });
    }

    void RendererBase::InitCommands()
    {
        vk::CommandPoolCreateInfo cmdPoolCI{};
        cmdPoolCI.setQueueFamilyIndex(mQueueFamilyIndex);
        cmdPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        mCommandPool = mDevice.createCommandPool(cmdPoolCI);

        vk::CommandBufferAllocateInfo cmdBufferAI{};
        cmdBufferAI.setCommandPool(mCommandPool);
        cmdBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
        cmdBufferAI.setCommandBufferCount(1);

        mCommandBuffer = mDevice.allocateCommandBuffers(cmdBufferAI).front();
    }

    void RendererBase::InitPipelines()
    {
        vk::ShaderModule computeDraw;
        if (!LoadShaderModule("../Assets/Shaders/Generated/ComputeDraw.comp.spv", &computeDraw))
        {
            throw std::runtime_error("Failed to load compute shader");
        }

        vk::PipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.setSetLayoutCount(1);
        pipelineLayoutCI.setPSetLayouts(&mDescriptorSetLayout);

        mPipelineLayout = mDevice.createPipelineLayout(pipelineLayoutCI);

        vk::PipelineShaderStageCreateInfo shaderStageCI{};
        shaderStageCI.setStage(vk::ShaderStageFlagBits::eCompute);
        shaderStageCI.setModule(computeDraw);
        shaderStageCI.setPName("main");

        vk::ComputePipelineCreateInfo pipelineCI{};
        pipelineCI.setStage(shaderStageCI);
        pipelineCI.setLayout(mPipelineLayout);

        mPipeline = mDevice.createComputePipeline(nullptr, pipelineCI).value;

        mDevice.destroyShaderModule(computeDraw);
    }

    void RendererBase::InitDescriptors()
    {
        std::vector<vk::DescriptorPoolSize> poolSizes = {
            {vk::DescriptorType::eStorageImage, 1}
        };

        vk::DescriptorPoolCreateInfo descPoolCI {};
        descPoolCI.setPoolSizes(poolSizes);
        descPoolCI.setMaxSets(10);

        mDescriptorPool = mDevice.createDescriptorPool(descPoolCI);

        vk::DescriptorSetLayoutBinding descSetLayoutBinding {};
        descSetLayoutBinding.setBinding(0);
        descSetLayoutBinding.setDescriptorType(vk::DescriptorType::eStorageImage);
        descSetLayoutBinding.setDescriptorCount(1);
        descSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eCompute);

        vk::DescriptorSetLayoutCreateInfo descSetLayoutCI {};
        descSetLayoutCI.setBindings(descSetLayoutBinding);

        mDescriptorSetLayout = mDevice.createDescriptorSetLayout(descSetLayoutCI);

        vk::DescriptorSetAllocateInfo descSetAI {};
        descSetAI.setDescriptorPool(mDescriptorPool);
        descSetAI.setSetLayouts(mDescriptorSetLayout);

        mDescriptorSet = mDevice.allocateDescriptorSets(descSetAI).front();

        vk::DescriptorImageInfo descImageInfo {};
        descImageInfo.setImageLayout(vk::ImageLayout::eGeneral);
        descImageInfo.setImageView(mDrawImageView);

        vk::WriteDescriptorSet descWrite {};
        descWrite.setDstSet(mDescriptorSet);
        descWrite.setDstBinding(0);
        descWrite.setDescriptorType(vk::DescriptorType::eStorageImage);
        descWrite.setImageInfo(descImageInfo);

        mDevice.updateDescriptorSets(descWrite, nullptr);
    }

    void RendererBase::InitSyncStructures()
    {
        vk::FenceCreateInfo fenceCI{};
        fenceCI.setFlags(vk::FenceCreateFlagBits::eSignaled);
        mImmdiateFence = mDevice.createFence(fenceCI);

        vk::SemaphoreCreateInfo semaphoreCI{};
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
        imageBarrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        vk::DependencyInfo dependencyInfo{};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &imageBarrier;

        cmdBuffer.pipelineBarrier2(dependencyInfo);
    }

    void RendererBase::TransitionImage(vk::CommandBuffer cmdBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
    {
        vk::ImageMemoryBarrier2 imageBarrier{};
        imageBarrier.setSrcStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
        imageBarrier.setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite);
        imageBarrier.setDstStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
        imageBarrier.setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead);
        imageBarrier.oldLayout = oldLayout;
        imageBarrier.newLayout = newLayout;

        vk::ImageSubresourceRange subImage {};
        subImage.aspectMask = (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
        subImage.levelCount = 1;
        subImage.layerCount = 1;
        subImage.baseMipLevel = 0;
        subImage.baseArrayLayer = 0;

        imageBarrier.image = image;
        imageBarrier.subresourceRange = subImage;

        vk::DependencyInfo dependencyInfo{};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &imageBarrier;

        cmdBuffer.pipelineBarrier2(dependencyInfo);
    }

    void RendererBase::CopyImage(vk::CommandBuffer cmdBuffer, vk::Image srcImage, vk::Image dstImage, vk::Extent3D extent)
    {
        vk::ImageSubresourceRange subImage {};
        subImage.aspectMask = vk::ImageAspectFlagBits::eColor;
        subImage.levelCount = 1;
        subImage.layerCount = 1;
        subImage.baseMipLevel = 0;
        subImage.baseArrayLayer = 0;

        vk::ImageCopy2 imageCopy {};
        imageCopy.extent = extent;
        imageCopy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageCopy.srcSubresource.layerCount = 1;
        imageCopy.srcSubresource.baseArrayLayer = 0;
        imageCopy.srcSubresource.mipLevel = 0;
        imageCopy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageCopy.dstSubresource.layerCount = 1;
        imageCopy.dstSubresource.baseArrayLayer = 0;
        imageCopy.dstSubresource.mipLevel = 0;

        vk::CopyImageInfo2 copyImageInfo {};
        copyImageInfo.srcImage = srcImage;
        copyImageInfo.dstImage = dstImage;
        copyImageInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
        copyImageInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
        copyImageInfo.setRegions(imageCopy);
        
        cmdBuffer.copyImage2(copyImageInfo);
    }

    bool RendererBase::LoadShaderModule(const char *filePath, vk::ShaderModule *outShaderModule)
    {
        // Open file, with cursor at the end
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) { return false; }

        // 获取文件大小，因为指针在末尾，所以指针位置就是文件大小
        size_t fileSize = (size_t)file.tellg();
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        file.close();

        vk::ShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.setCodeSize(buffer.size() * sizeof(uint32_t));
        shaderModuleCI.setPCode(buffer.data());

        *outShaderModule = mDevice.createShaderModule(shaderModuleCI);

        return true;
    }
} // namespace Vultana::Renderer
