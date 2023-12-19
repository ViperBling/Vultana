#pragma once

#include "RHI/RHIDevice.hpp"
#include "Utilities/Utility.hpp"

#include <vulkan/vulkan.hpp>

struct VmaAllocator_T;
struct VmaAllocation_T;
using VmaAllocator = VmaAllocator_T*;
using VmaAllocation = VmaAllocation_T*;

namespace Vultana
{
    class VKDevice : public RHIDevice
    {
    public:
        NOCOPY(VKDevice);
        VKDevice(const RHIDeviceInfo& deviceInfo);
        virtual void OnCreate() override;
        virtual void OnDestroy() override;

        virtual void* GetHandle() const override { return nullptr; }

        virtual void BeginFrame() override {}
        virtual void EndFrame() override {}

        virtual bool Init() override { return false; }

    private:
        RHIDeviceInfo mDeviceInfo;

        vk::PhysicalDevice mPhysicalDevice;
        vk::PhysicalDeviceMemoryProperties mPDMemoryProps;
        vk::PhysicalDeviceProperties mPDProps;

        vk::Device mDevice;
        vk::Instance mInstance;
        vk::SurfaceKHR mSurface;

        vk::Queue mGraphicsQueue;
        uint32_t mGraphicsQueueFamilyIndex;
        vk::Queue mPresentQueue;
        uint32_t mPresentQueueFamilyIndex;

        VmaAllocator mAllocator;
    };

    
} // namespace Vultana::RHI
