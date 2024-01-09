#pragma once

#include "Utilities/Utility.hpp"

#include <iostream>

#include <vulkan/vulkan.hpp>

namespace RHI::Vulkan
{
    class InstanceVK
    {
    public:
        InstanceVK();
        ~InstanceVK();

        void Destroy();

        vk::Instance GetInstance() const { return mInstance; }
        vk::PhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }
        vk::DebugUtilsMessengerEXT GetDebugMessenger() const { return mDebugMessenger; }
        vk::DispatchLoaderDynamic GetDynamicLoader() const { return mDynamicLoader; }

    private:
        void AddInstanceLayers();
        void AddInstanceExtensions();
        void CreateInstance();
        
        void EnumeratePhysicalDevices();

    private:
        vk::Instance mInstance;
        vk::PhysicalDevice mPhysicalDevice;
        std::vector<const char*> mInstanceExtensions;
        std::vector<const char*> mInstanceLayers;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        vk::DispatchLoaderDynamic mDynamicLoader;
    };
}