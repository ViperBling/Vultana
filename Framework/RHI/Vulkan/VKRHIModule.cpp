#include "VKRHIModule.hpp"
#include "VKInstance.hpp"

namespace Vultana
{
    RHIInstance *GetInstance()
    {
        static VKInstance vkInstance;
        return &vkInstance;
    }

    VulkanRHIModule *VulkanRHIModule::GetModule()
    {
        static VulkanRHIModule vkRHIModule;
        return &vkRHIModule;
    }
}