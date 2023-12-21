#pragma once

#include "Utilities/Utility.hpp"
#include "RHI/RHIInstance.hpp"

#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class InstanceVK : public RHIInstance
    {
    public:
        NOCOPY(InstanceVK)
        InstanceVK();
        ~InstanceVK() override;

        RHIRenderBackend GetRHIBackend() override { return RHIRenderBackend::Vulkan; }
        vk::Instance GetVkInstance() const;
        void Destroy() override;

    private:

    private:
        vk::Instance mInstance;
        std::vector<const char*> mInstanceExtensions;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        
    };
}