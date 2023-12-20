#include "RHIInstance.hpp"
#include "RHI/RHIVulkan/VKInstance.hpp"

namespace Vultana
{
    RHIInstance *RHIInstance::GetInstanceByRHIBackend(const RHIRenderBackend &backend)
    {
        RHIInstance* instance = nullptr;

        switch (backend)
        {
        case RHIRenderBackend::Vulkan :
            instance = new VKInstance();
            break;
        default:
            break;
        }
        return instance;
    }
    
}