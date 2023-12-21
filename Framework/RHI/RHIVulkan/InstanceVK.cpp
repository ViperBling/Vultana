#include "InstanceVK.hpp"

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

    InstanceVK::InstanceVK()
    {
    }

    InstanceVK::~InstanceVK()
    {
    }

    vk::Instance InstanceVK::GetVkInstance() const
    {
        return vk::Instance();
    }

    void InstanceVK::Destroy()
    {
    }

} // namespace Vultana::RHI
