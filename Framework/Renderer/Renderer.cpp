#include "Renderer.hpp"
#include "RHI/RHICommon.hpp"
#include "Windows/GLFWindow.hpp"

#include <optional>

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

    void RendererBase::Init(void* windowHandle)
    {
        mWndHandle = static_cast<GLFWindow*>(windowHandle);
        
        CreateInstance();
        PickPhysicalDevice();
        CreateLogicalDevice();
        SetupDebugMessenger();
        CreateSwapchain();
        CreateImageViews();
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

    void RendererBase::CreateSwapchain()
    {
        
    }

    void RendererBase::CreateImageViews()
    {
    }

    void RendererBase::CreateRenderPass()
    {
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
