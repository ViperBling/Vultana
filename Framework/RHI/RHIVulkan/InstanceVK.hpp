#pragma once

#include "Utilities/Utility.hpp"
#include "RHI/RHIInstance.hpp"
#include "RHI/RHIGPU.hpp"

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
        uint32_t GetGPUCount() override { return static_cast<uint32_t>(mGPUs.size()); }
        RHIGPU* GetGPU(uint32_t index) override { return &mGPUs[index]; }

        vk::Instance GetVkInstance() const;
        void Destroy() override;

        void DebugOutput() override;

    private:
        void AddInstanceLayers();
        void AddInstanceExtensions();
        void CreateInstance();

        void EnumeratePhysicalDevices();

    private:
        vk::Instance mInstance;
        std::vector<const char*> mInstanceExtensions;
        std::vector<const char*> mInstanceLayers;
        std::vector<RHIGPU> mGPUs;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        vk::DispatchLoaderDynamic mDynamicLoader;
    };
}