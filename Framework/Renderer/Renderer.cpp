#include "Renderer.hpp"
#include "RHI/RHICommon.hpp"
#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

#include <vk_mem_alloc.h>

namespace Vultana
{
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

    static std::vector<char> readFiles(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file!" + filename);
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    void CheckRequestedLayers(const RendererCreateInfo& createInfo)
    {
        createInfo.InfoCallback("Enumerating requested layers:");

        auto layers = vk::enumerateInstanceLayerProperties();
        for (const char* layerName : createInfo.ValidationLayers)
        {
            createInfo.InfoCallback("- " + std::string(layerName));

            auto layerIt = std::find_if(layers.begin(), layers.end(),
                [layerName](const vk::LayerProperties& layer)
                {
                    return std::strcmp(layer.layerName.data(), layerName) == 0;
                });

            if (layerIt == layers.end())
            {
                createInfo.ErrorCallback(("Cannot enable requested layer: " + std::string(layerName)).c_str());
                return;
            }
        }
    }

    void CheckRequestedExtensions(const RendererCreateInfo& createInfo)
    {
        createInfo.InfoCallback("Enumerating requested extensions...");
        auto extensions = vk::enumerateInstanceExtensionProperties();

        for (const char* exetensionName : createInfo.Extensions)
        {
            createInfo.InfoCallback("- " + std::string(exetensionName));

            auto layerIt = std::find_if(extensions.begin(), extensions.end(), 
                [exetensionName](const vk::ExtensionProperties& extension) 
                {
                    return std::strcmp(extension.extensionName.data(), exetensionName) == 0;
                });
            
            if (layerIt == extensions.end())
            {
                createInfo.ErrorCallback("Cannot enable requested extension!");
                return;
            }
        }
    }

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

    RendererBase::RendererBase(const RendererCreateInfo& createInfo)
    {
        vk::ApplicationInfo appInfo {};
        appInfo.pApplicationName = createInfo.ApplicationName;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = createInfo.ApplicationName;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        auto extensions = createInfo.Extensions;
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        vk::InstanceCreateInfo instanceCI {};
        instanceCI.setPApplicationInfo(&appInfo);
        instanceCI.setPEnabledExtensionNames(extensions);
        instanceCI.setPEnabledLayerNames(createInfo.ValidationLayers);

        CheckRequestedExtensions(createInfo);
        CheckRequestedLayers(createInfo);

        mInstance = vk::createInstance(instanceCI);
        
        createInfo.InfoCallback("Created Vulkan instance.");
    }

    RendererBase::~RendererBase()
    {
        mDevice.waitIdle();

        if (mCommandPool) mDevice.destroyCommandPool(mCommandPool);

        mSwapchainImages.clear();
        mSwapchainImageViews.clear();

        vmaDestroyAllocator(mAllocator);

        if (mSwapchain) mDevice.destroySwapchainKHR(mSwapchain);
        if (mImageAvailableSemaphore) mDevice.destroySemaphore(mImageAvailableSemaphore);
        if (mRenderFinishedSemaphore) mDevice.destroySemaphore(mRenderFinishedSemaphore);
        if (mImmdiateFence) mDevice.destroyFence(mImmdiateFence);
        if (mDevice) mDevice.destroy();
        if (mDebugMessenger) mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger, nullptr, mDynamicLoader);
        if (mSurface) mInstance.destroySurfaceKHR(mSurface);
        if (mInstance) mInstance.destroy();

        mPresentImageCount = 0;
        mQueueFamilyIndex = 0;        
    }

    void RendererBase::Init(const vk::SurfaceKHR& surface, RendererCreateInfo& createInfo)
    {
        mSurface = *reinterpret_cast<const VkSurfaceKHR*>(&surface);

        if (!mSurface)
        {
            createInfo.ErrorCallback("Failed to initialize surface!");
            return;
        }

        // Physical Device
        createInfo.InfoCallback("Enumerating physical devices...");
        auto pds = mInstance.enumeratePhysicalDevices();
        for (const auto& pd : pds)
        {
            auto props = pd.getProperties();
            createInfo.InfoCallback("- Checking " + std::string(props.deviceName.data()) + "...");

            auto queueFamilyIndex = DetermineQueueFamilyIndex(mInstance, pd, mSurface);
            if (!queueFamilyIndex.has_value())
            {
                createInfo.InfoCallback(std::string(props.deviceName.data()) + ": skipping device as its queue families does not satisfy the requirements");
                continue;
            }
            
            mPhysicalDevice = pd;
            mPDProps = props;
            mQueueFamilyIndex = queueFamilyIndex.value();
            if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                break;
            }
        }

        if (!mPhysicalDevice)
        {
            createInfo.ErrorCallback("Failed to find suitable physical device!");
            return;
        }

        // Surface Present Info
        auto presentModes = mPhysicalDevice.getSurfacePresentModesKHR(mSurface);
        auto surfaceFormats = mPhysicalDevice.getSurfaceFormatsKHR(mSurface);
        auto surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);

        mPresentMode = vk::PresentModeKHR::eImmediate;
        if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end())
        {
            mPresentMode = vk::PresentModeKHR::eMailbox;
        }
        mPresentImageCount = std::max(surfaceCapabilities.maxImageCount, 1u);

        mSurfaceFormat = surfaceFormats.front();
        for (const auto& fmt : surfaceFormats)
        {
            if (fmt.format == vk::Format::eR8G8B8A8Unorm && fmt.format == vk::Format::eB8G8R8A8Unorm)
            {
                mSurfaceFormat = fmt;
                break;
            }
        }
        createInfo.InfoCallback("Selected surface present mode: " + std::string(vk::to_string(mPresentMode)));
        createInfo.InfoCallback("Selected surface format: " + std::string(vk::to_string(mSurfaceFormat.format)));

        vk::DeviceQueueCreateInfo queueCI {};
        std::array queuePriorities = {1.0f};
        queueCI.setQueueFamilyIndex(mQueueFamilyIndex);
        queueCI.setQueueCount(1);
        queueCI.setQueuePriorities(queuePriorities);

        auto deviceExtensions = createInfo.DeviceExtensions;
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        vk::DeviceCreateInfo deviceCI {};
        deviceCI.setQueueCreateInfos(queueCI);
        deviceCI.setPEnabledExtensionNames(deviceExtensions);

        mDevice = mPhysicalDevice.createDevice(deviceCI);
        mQueue = mDevice.getQueue(mQueueFamilyIndex, 0);

        createInfo.InfoCallback("Created Vulkan device.");

        mDynamicLoader.init(mInstance, mDevice);

        vk::DebugUtilsMessengerCreateInfoEXT debugCI {};
        debugCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        debugCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugCI.setPfnUserCallback(ValidationLayerCallback);
        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugCI, nullptr, mDynamicLoader);

        VmaAllocatorCreateInfo allocateCI {};
        allocateCI.physicalDevice = mPhysicalDevice;
        allocateCI.device = mDevice;
        allocateCI.instance = mInstance;
        vmaCreateAllocator(&allocateCI, &mAllocator);
        createInfo.InfoCallback("Created Vulkan allocator.");

        RecreateSwapchian(surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.maxImageExtent.height);
        createInfo.InfoCallback("Created Vulkan swapchain.");

        mImageAvailableSemaphore = mDevice.createSemaphore({});
        mRenderFinishedSemaphore = mDevice.createSemaphore({});
        mImmdiateFence = mDevice.createFence(vk::FenceCreateInfo{ });
        
        vk::CommandPoolCreateInfo commandPoolCI {};
        commandPoolCI.setQueueFamilyIndex(mQueueFamilyIndex);
        commandPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);
        mCommandPool = mDevice.createCommandPool(commandPoolCI);

        vk::CommandBufferAllocateInfo commandBufferAI {};
        commandBufferAI.setCommandPool(mCommandPool);
        commandBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
        commandBufferAI.setCommandBufferCount(1);
        mCommandBuffer = mDevice.allocateCommandBuffers(commandBufferAI).front();

        createInfo.InfoCallback("Created Vulkan command pool and command buffer.");

        CreateRenderPass();
        CreatePipeline();
        CreateFramebuffer();
    }

    void RendererBase::RecreateSwapchian(uint32_t width, uint32_t height)
    {
        mDevice.waitIdle();

        auto surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
        mSurfaceExtent = vk::Extent2D(
            std::clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            std::clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
        );

        vk::SwapchainCreateInfoKHR swapchainCI {};
        swapchainCI.setSurface(mSurface);
        swapchainCI.setMinImageCount(mPresentImageCount);
        swapchainCI.setImageFormat(mSurfaceFormat.format);
        swapchainCI.setImageColorSpace(mSurfaceFormat.colorSpace);
        swapchainCI.setImageExtent(mSurfaceExtent);
        swapchainCI.setImageArrayLayers(1);
        swapchainCI.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
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

        auto swapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);
        mPresentImageCount = swapchainImages.size();
        mSwapchainImages.clear();
        mSwapchainImages.reserve(mPresentImageCount);
        mSwapchainImageViews.resize(swapchainImages.size());

        for (size_t i = 0; i < mSwapchainImages.size(); i++)
        {
            vk::ImageViewCreateInfo imageViewCI {};
            imageViewCI.setImage(swapchainImages[i]);
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            imageViewCI.setFormat(mSurfaceFormat.format);
            imageViewCI.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
            mSwapchainImageViews[i] = mDevice.createImageView(imageViewCI);
        }
    }

    void RendererBase::RenderFrame()
    {
        vk::Result res = mDevice.waitForFences(mImmdiateFence, true, UINT64_MAX);
        if (res != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to wait for fence!");
        }

        uint32_t imageIndex = mDevice.acquireNextImageKHR(mSwapchain, UINT64_MAX, mImageAvailableSemaphore, {}).value;

        mDevice.resetFences(mImmdiateFence);
        mCommandBuffer.reset({});
        
        RecreateSwapchian(mSurfaceExtent.width, mSurfaceExtent.height);
        CreateFramebuffer();

        vk::CommandBufferBeginInfo commandBufferBI {};

        mCommandBuffer.begin(commandBufferBI);

        vk::RenderPassBeginInfo renderPassBI {};
        renderPassBI.setRenderPass(mRenderPass);
        renderPassBI.setFramebuffer(mFramebuffers[imageIndex]);
        renderPassBI.setRenderArea({ { 0, 0 }, mSurfaceExtent });

        std::array<vk::ClearValue, 1> clearValues = { vk::ClearColorValue{ std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } } };
        renderPassBI.setClearValues(clearValues);

        mCommandBuffer.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
        mCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);

        vk::Viewport viewport {};
        viewport.setWidth(mSurfaceExtent.width);
        viewport.setHeight(mSurfaceExtent.height);
        viewport.setMinDepth(0.0f);
        viewport.setMaxDepth(1.0f);
        mCommandBuffer.setViewport(0, viewport);
        vk::Rect2D scissor {};
        scissor.setExtent(mSurfaceExtent);
        mCommandBuffer.setScissor(0, scissor);

        mCommandBuffer.draw(3, 1, 0, 0);
        mCommandBuffer.endRenderPass();

        mCommandBuffer.end();

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::Semaphore waitSem = mImageAvailableSemaphore;
        vk::Semaphore signalSem = mRenderFinishedSemaphore;

        vk::SubmitInfo submitInfo {};
        submitInfo.setWaitSemaphores(waitSem);
        submitInfo.setWaitDstStageMask(waitStage);
        submitInfo.setCommandBuffers(mCommandBuffer);
        submitInfo.setSignalSemaphores(signalSem);

        mQueue.submit(submitInfo, mImmdiateFence);

        vk::PresentInfoKHR presentInfo {};
        presentInfo.setWaitSemaphores(signalSem);
        presentInfo.setSwapchains(mSwapchain);
        presentInfo.setImageIndices(imageIndex);

        res = mQueue.presentKHR(presentInfo);
        if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR)
        {
            RecreateSwapchian(mSurfaceExtent.width, mSurfaceExtent.height);
            CreateFramebuffer();
        }
        else if (res != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to present swapchain image!");
        }
    }

    void RendererBase::CreateRenderPass()
    {
        // Render Pass
        vk::AttachmentDescription colorAttach {};
        colorAttach.setFormat(mSurfaceFormat.format);
        colorAttach.setSamples(vk::SampleCountFlagBits::e1);
        colorAttach.setLoadOp(vk::AttachmentLoadOp::eClear);
        colorAttach.setStoreOp(vk::AttachmentStoreOp::eStore);
        colorAttach.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        colorAttach.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        colorAttach.setInitialLayout(vk::ImageLayout::eUndefined);
        colorAttach.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentReference colorAttachRef {};
        colorAttachRef.setAttachment(0);
        colorAttachRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription subpass {};
        subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
        subpass.setColorAttachments(colorAttachRef);

        vk::SubpassDependency dependency {};
        dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
        dependency.setDstSubpass(0);
        dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dependency.setSrcAccessMask({});
        dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

        vk::RenderPassCreateInfo renderPassCI {};
        renderPassCI.setAttachments(colorAttach);
        renderPassCI.setSubpasses(subpass);
        renderPassCI.setDependencies(dependency);

        mRenderPass = mDevice.createRenderPass(renderPassCI);
    }

    void RendererBase::CreatePipeline()
    {
        auto vsCode = readFiles("./Generated/Triangle.vert.spv");
        auto fsCode = readFiles("./Generated/Triangle.frag.spv");

        vk::ShaderModuleCreateInfo vsCI {};
        vsCI.pCode = reinterpret_cast<const uint32_t*>(vsCode.data());
        vsCI.codeSize = vsCode.size();

        vk::ShaderModuleCreateInfo fsCI {};
        fsCI.pCode = reinterpret_cast<const uint32_t*>(fsCode.data());
        fsCI.codeSize = fsCode.size();

        vk::ShaderModule vsModule = mDevice.createShaderModule(vsCI);
        vk::ShaderModule fsModule = mDevice.createShaderModule(fsCI);

        vk::PipelineShaderStageCreateInfo vsStageCI {};
        vsStageCI.setStage(vk::ShaderStageFlagBits::eVertex);
        vsStageCI.setModule(vsModule);
        vsStageCI.setPName("main");

        vk::PipelineShaderStageCreateInfo fsStageCI {};
        fsStageCI.setStage(vk::ShaderStageFlagBits::eFragment);
        fsStageCI.setModule(fsModule);
        fsStageCI.setPName("main");

        vk::PipelineShaderStageCreateInfo shaderStages[] = { vsStageCI, fsStageCI };

        vk::PipelineVertexInputStateCreateInfo viStateCI {};
        viStateCI.setVertexBindingDescriptions({ });
        viStateCI.setVertexAttributeDescriptions({ });

        vk::PipelineInputAssemblyStateCreateInfo iaStateCI {};
        iaStateCI.setTopology(vk::PrimitiveTopology::eTriangleList);
        iaStateCI.setPrimitiveRestartEnable(false);

        vk::PipelineViewportStateCreateInfo vpStateCI {};
        vpStateCI.setViewportCount(1);
        vpStateCI.setScissorCount(1);

        vk::PipelineRasterizationStateCreateInfo rsStateCI {};
        rsStateCI.setDepthClampEnable(false);
        rsStateCI.setRasterizerDiscardEnable(false);
        rsStateCI.setPolygonMode(vk::PolygonMode::eFill);
        rsStateCI.setLineWidth(1.0f);
        rsStateCI.setCullMode(vk::CullModeFlagBits::eBack);
        rsStateCI.setFrontFace(vk::FrontFace::eClockwise);
        rsStateCI.setDepthBiasEnable(false);


        vk::PipelineMultisampleStateCreateInfo msStateCI {};
        msStateCI.setRasterizationSamples(vk::SampleCountFlagBits::e1);
        msStateCI.setSampleShadingEnable(false);
        msStateCI.setMinSampleShading(1.0f);
        msStateCI.setAlphaToCoverageEnable(false);
        msStateCI.setAlphaToOneEnable(false);

        vk::PipelineColorBlendAttachmentState cbAttach {};
        cbAttach.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        cbAttach.setBlendEnable(false);

        vk::PipelineColorBlendStateCreateInfo cbStateCI {};
        cbStateCI.setLogicOpEnable(false);
        cbStateCI.setAttachments(cbAttach);

        std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynStateCI {};
        dynStateCI.setDynamicStates(dynamicStates);

        vk::PipelineLayoutCreateInfo layoutCI {};
        layoutCI.setSetLayouts({ });
        layoutCI.setPushConstantRanges({ });

        mPipelineLayout = mDevice.createPipelineLayout(layoutCI);

        vk::GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.setStages(shaderStages);
        pipelineCI.setPVertexInputState(&viStateCI);
        pipelineCI.setPInputAssemblyState(&iaStateCI);
        pipelineCI.setPViewportState(&vpStateCI);
        pipelineCI.setPRasterizationState(&rsStateCI);
        pipelineCI.setPMultisampleState(&msStateCI);
        pipelineCI.setPColorBlendState(&cbStateCI);
        pipelineCI.setPDynamicState(&dynStateCI);
        pipelineCI.setLayout(mPipelineLayout);
        pipelineCI.setRenderPass(mRenderPass);
        pipelineCI.setSubpass(0);

        mGraphicsPipeline = mDevice.createGraphicsPipeline(nullptr, pipelineCI).value;

        mDevice.destroyShaderModule(vsModule);
        mDevice.destroyShaderModule(fsModule);
    }

    void RendererBase::CreateFramebuffer()
    {
        mFramebuffers.resize(mSwapchainImages.size());

        for (size_t i = 0; i < mFramebuffers.size(); i++)
        {
            vk::FramebufferCreateInfo framebufferCI {};
            framebufferCI.setRenderPass(mRenderPass);
            framebufferCI.setAttachments(mSwapchainImageViews[i]);
            framebufferCI.setWidth(mSurfaceExtent.width);
            framebufferCI.setHeight(mSurfaceExtent.height);
            framebufferCI.setLayers(1);
            mFramebuffers[i] = mDevice.createFramebuffer(framebufferCI);
        }
    }

} // namespace Vultana::Renderer
