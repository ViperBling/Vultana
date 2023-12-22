#include "InstanceVK.hpp"
#include "RHICommonVK.hpp"
#include "GPUVK.hpp"

#include <vector>

namespace Vultana
{
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

    InstanceVK::InstanceVK() : RHIInstance()
    {
        AddInstanceLayers();
        AddInstanceExtensions();
        CreateInstance();
        EnumeratePhysicalDevices();
    }

    InstanceVK::~InstanceVK()
    {
        Destroy();
    }

    void InstanceVK::Destroy()
    {
        mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger, nullptr, mDynamicLoader);
        mInstance.destroy();
    }

    void InstanceVK::DebugOutput()
    {
        for (auto & layer : mInstanceLayers)
        {
            std::cout << std::string(layer) << std::endl;
        }
        for (auto & extension : mInstanceExtensions)
        {
            std::cout << std::string(extension) << std::endl;
        }
        for (auto& gpu : mGPUs)
        {
            auto name = static_cast<GPUVK*>(gpu.get())->GetVKPhysicalDevice().getProperties().deviceName;
            std::cout << name << std::endl;
        }
    }

    void InstanceVK::AddInstanceLayers()
    {
        static std::vector<const char *> requestLayers = {
            VK_KRONOS_VALIDATION_LAYER_NAME,
        };

        auto instanceLayers = vk::enumerateInstanceLayerProperties();

        for (auto &layer : requestLayers)
        {
            auto it = std::find_if(instanceLayers.begin(), instanceLayers.end(), [&layer](const auto &layerProp) -> bool
            {
                return std::string(layer) == layerProp.layerName;
            });
            if (it == instanceLayers.end()) { continue; }
            mInstanceLayers.emplace_back(layer);
        }
    }

    void InstanceVK::AddInstanceExtensions()
    {
        static std::vector<const char *> requestExtensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            "VK_KHR_win32_surface",
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };

        auto instanceExtenstions = vk::enumerateInstanceExtensionProperties();

        for (auto& extension : requestExtensions)
        {
            auto it = std::find_if(instanceExtenstions.begin(), instanceExtenstions.end(), [&extension](const auto& extensionProp) -> bool
            {
                return std::string(extension) == extensionProp.extensionName;
            });
            if (it == instanceExtenstions.end()) { continue; }
            mInstanceExtensions.emplace_back(extension);
        }
    }

    void InstanceVK::CreateInstance()
    {
        vk::ApplicationInfo appInfo {};
        appInfo.pApplicationName = "Vultana";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Vultana";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        vk::InstanceCreateInfo instanceCI {};
        instanceCI.pApplicationInfo = &appInfo;
        instanceCI.setPEnabledLayerNames(mInstanceLayers);
        instanceCI.setPEnabledExtensionNames(mInstanceExtensions);

        mInstance = vk::createInstance(instanceCI);

        mDynamicLoader.init(mInstance);

        vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCI{};
        debugMessengerCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        debugMessengerCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugMessengerCI.setPfnUserCallback(ValidationLayerCallback);
        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugMessengerCI, nullptr, mDynamicLoader);
    }

    void InstanceVK::EnumeratePhysicalDevices()
    {
        auto physicalDevices = mInstance.enumeratePhysicalDevices();

        mGPUs.resize(physicalDevices.size());
        for (size_t i = 0; i < physicalDevices.size(); ++i)
        {
            mGPUs[i] = std::make_unique<GPUVK>(*this, physicalDevices[i]);
        }
    }

} // namespace Vultana::RHI
