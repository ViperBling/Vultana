#include "RHIInstance.hpp"
#include "RHI/RHIVulkan/InstanceVK.hpp"

namespace RHI
{
    RHIInstance *RHIInstance::GetInstanceByRHIBackend(const RHIRenderBackend &backend)
    {
        RHIInstance* instance = nullptr;

        switch (backend)
        {
        case RHIRenderBackend::Vulkan :
            instance = new InstanceVK();
            break;
        default:
            break;
        }
        return instance;
    }
    
}