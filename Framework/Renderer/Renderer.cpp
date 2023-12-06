#include "Renderer.hpp"
#include "RHI/RHICommon.hpp"
#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>

#include <vk_mem_alloc.h>

namespace Vultana
{
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
    }

    void RendererBase::RecreateSwapchian(uint32_t width, uint32_t height)
    {

    }

    void RendererBase::RenderFrame()
    {

    }

} // namespace Vultana::Renderer
