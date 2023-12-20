#pragma once

#include "Utilities/Utility.hpp"
#include "RHI/RHIInstance.hpp"

#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class VKInstance : public RHIInstance
    {
    public:
        NOCOPY(VKInstance)
        VKInstance();
        ~VKInstance() override;

        RHIRenderBackend GetRHIBackend() override;
        void Destroy() override;
    };
}