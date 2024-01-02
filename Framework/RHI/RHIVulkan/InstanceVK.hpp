#pragma once

#include "Utilities/Utility.hpp"
#include "RHI/RHIInstance.hpp"
#include "RHI/RHIGPU.hpp"

#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    class InstanceVK : public RHIInstance
    {
    public:
        NOCOPY(InstanceVK)
        InstanceVK();
        ~InstanceVK() override;

        RHIRenderBackend GetRHIBackend() override { return RHIRenderBackend::Vulkan; }
        uint32_t GetGPUCount() override { return static_cast<uint32_t>(mGPUs.size()); }
        RHIGPU* GetGPU(uint32_t index) override { return mGPUs[index].get(); }

        vk::Instance GetVkInstance() const { return mInstance; }
        vk::DispatchLoaderDynamic GetVkDynamicLoader() const { return mDynamicLoader; }
        void Destroy() override;

    private:
        void AddInstanceLayers();
        void AddInstanceExtensions();
        void CreateInstance();

        void EnumeratePhysicalDevices();

    private:
        vk::Instance mInstance;
        std::vector<const char*> mInstanceExtensions;
        std::vector<const char*> mInstanceLayers;
        std::vector<std::unique_ptr<RHIGPU>> mGPUs;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        vk::DispatchLoaderDynamic mDynamicLoader;
    };
}