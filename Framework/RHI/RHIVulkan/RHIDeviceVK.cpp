#include "RHIDeviceVK.hpp"

#include "Utilities/Log.hpp"

#include <map>

namespace RHI
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) 
    {
        VTNA_LOG_DEBUG(pCallbackData->pMessage);
        return VK_FALSE;
    }

    RHIDeviceVK::RHIDeviceVK(const RHIDeviceDesc &desc)
    {
        mDesc = desc;
    }

    RHIDeviceVK::~RHIDeviceVK()
    {
        for (size_t i = 0; i < RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            delete mTransitionCopyCmdList[i];
            delete mTransitionGraphicsCmdList[i];
        }
        delete mDeferredDeletionQueue;

        vmaDestroyAllocator(mAllocator);
        mDevice.destroy();
        mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger);
        mInstance.destroy();
    }

    bool RHIDeviceVK::Initialize()
    {
        assert(CreateInstance());
        assert(CreateDevice());
        assert(CreateVmaAllocator());

        mDeferredDeletionQueue = new RHIDeletionQueueVK(this);

        for (size_t i = 0; i < RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            mTransitionCopyCmdList[i] = CreateCommandList(ERHICommandQueueType::Copy, "Transition CmdList(Copy)");
            mTransitionGraphicsCmdList[i] = CreateCommandList(ERHICommandQueueType::Graphics, "Transition CmdList(Graphics)");
        }

        return true;
    }

    void RHIDeviceVK::BeginFrame()
    {
    }

    void RHIDeviceVK::EndFrame()
    {
    }

    RHISwapchain *RHIDeviceVK::CreateSwapchain(const RHISwapchainDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHICommandList *RHIDeviceVK::CreateCommandList(ERHICommandQueueType queueType, const std::string &name)
    {
        return nullptr;
    }

    RHIFence *RHIDeviceVK::CreateFence(const std::string &name)
    {
        return nullptr;
    }

    RHIHeap *RHIDeviceVK::CreateHeap(const RHIHeapDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIBuffer *RHIDeviceVK::CreateBuffer(const RHIBufferDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHITexture *RHIDeviceVK::CreateTexture(const RHITextureDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIShader *RHIDeviceVK::CreateShader(const RHIShaderDesc &desc, tcb::span<uint8_t> data, const std::string &name)
    {
        return nullptr;
    }

    RHIPipelineState *RHIDeviceVK::CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIPipelineState *RHIDeviceVK::CreateComputePipelineState(const RHIComputePipelineStateDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIDescriptor *RHIDeviceVK::CreateShaderResourceView(RHIResource *resource, const RHIShaderResourceViewDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIDescriptor *RHIDeviceVK::CreateUnorderedAccessView(RHIResource *resource, const RHIUnorderedAccessViewDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIDescriptor *RHIDeviceVK::CreateConstantBufferView(RHIResource *resource, const RHIConstantBufferViewDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIDescriptor *RHIDeviceVK::CreateSampler(const RHISamplerDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    uint32_t RHIDeviceVK::GetAllocationSize(const RHIBufferDesc &desc) const
    {
        return 0;
    }

    uint32_t RHIDeviceVK::GetAllocationSize(const RHITextureDesc &desc) const
    {
        return 0;
    }

    bool RHIDeviceVK::DumpMemoryStats(const std::string &file)
    {
        return false;
    }

    void RHIDeviceVK::EnqueueDefaultLayoutTransition(RHITexture *texture)
    {
    }

    void RHIDeviceVK::CancelDefaultLayoutTransition(RHITexture *texture)
    {
    }

    void RHIDeviceVK::FlushLayoutTransition(ERHICommandQueueType queueType)
    {
    }

    bool RHIDeviceVK::CreateInstance()
    {
        auto supportExtens = vk::enumerateInstanceExtensionProperties();
        auto supportLayers = vk::enumerateInstanceLayerProperties();

        VTNA_LOG_DEBUG("Available instance layers:");
        for (uint32_t i = 0; i < supportLayers.size(); i++)
        {
            VTNA_LOG_DEBUG("  {}", supportLayers[i].layerName);
        }
        VTNA_LOG_DEBUG("Available instance extensions:");
        for (uint32_t i = 0; i < supportExtens.size(); i++)
        {
            VTNA_LOG_DEBUG("  {}", supportExtens[i].extensionName);
        }

        std::vector<const char*> requiredLayers = 
        {
            #if defined(_DEBUG) || defined(DEBUG)
            "VK_LAYER_KHRONOS_validation",
            #endif
        };

        std::vector<const char*> requiredExtensions = 
        {
            "VK_KHR_surface",
            "VK_KHR_win32_surface",
            "VK_EXT_debug_utils",
        };

        vk::ApplicationInfo appInfo {};
        appInfo.setPEngineName("Vultana");
        appInfo.setApiVersion(VK_HEADER_VERSION_COMPLETE);

        vk::InstanceCreateInfo instanceCI {};
        instanceCI.setPApplicationInfo(&appInfo);
        instanceCI.setPEnabledLayerNames(requiredLayers);
        instanceCI.setPEnabledExtensionNames(requiredExtensions);

        mInstance = vk::createInstance(instanceCI);

        if (mInstance != VK_NULL_HANDLE) return true;
        else return false;
    }

    bool RHIDeviceVK::CreateDevice()
    {
        auto physicalDevices = mInstance.enumeratePhysicalDevices();
        for (const auto & pd : physicalDevices)
        {
            auto properties = pd.getProperties();
            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                mPhysicalDevice = pd;
                break;
            }
        }
        auto properties = mPhysicalDevice.getProperties();

        VTNA_LOG_INFO("GPU : {}", properties.deviceName);
        VTNA_LOG_INFO("API Version : {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));

        auto physicalDeviceExtenProps = mPhysicalDevice.enumerateDeviceExtensionProperties();

        std::vector<const char*> requiredExtensions =
        {
            "VK_KHR_swapchain",
            "VK_KHR_swapchain_mutable_format",
            "VK_KHR_maintenance1",
            "VK_KHR_maintenance2",
            "VK_KHR_maintenance3",
            "VK_KHR_maintenance4",
            "VK_KHR_buffer_device_address",
            "VK_KHR_deferred_host_operations",
            "VK_KHR_acceleration_structure",
            // "VK_KHR_ray_query",
            "VK_KHR_dynamic_rendering",
            "VK_KHR_synchronization2",
            "VK_KHR_copy_commands2",
            "VK_KHR_bind_memory2",
            "VK_KHR_timeline_semaphore",
            "VK_KHR_dedicated_allocation",
            //"VK_EXT_swapchain_maintenance1",
            // "VK_EXT_mesh_shader",
            "VK_EXT_descriptor_indexing",
            "VK_EXT_mutable_descriptor_type",
            "VK_EXT_descriptor_buffer",
        };

        float queuePriorities[1] = {0.0};
        FindQueueFamilyIndex();

        vk::DeviceQueueCreateInfo queueCI[3] {};
        queueCI[0].setQueueFamilyIndex(mCopyQueueIndex);
        queueCI[0].setQueueCount(1);
        queueCI[0].setQueuePriorities(queuePriorities);
        queueCI[1].setQueueFamilyIndex(mGraphicsQueueIndex);
        queueCI[1].setQueueCount(1);
        queueCI[1].setQueuePriorities(queuePriorities);
        queueCI[2].setQueueFamilyIndex(mComputeQueueIndex);
        queueCI[2].setQueueCount(1);
        queueCI[2].setQueuePriorities(queuePriorities);

        vk::PhysicalDeviceFeatures features {};
        features = mPhysicalDevice.getFeatures();

        vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddress {};
        bufferDeviceAddress.setBufferDeviceAddress(VK_TRUE);

        vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphore {};
        timelineSemaphore.setTimelineSemaphore(VK_TRUE);
        timelineSemaphore.setPNext(&bufferDeviceAddress);

        vk::PhysicalDeviceSynchronization2Features sync2 {};
        sync2.setSynchronization2(VK_TRUE);
        sync2.setPNext(&timelineSemaphore);

        vk::PhysicalDeviceDynamicRenderingFeatures dynamicRendering {};
        dynamicRendering.setDynamicRendering(VK_TRUE);
        dynamicRendering.setPNext(&sync2);

        vk::DeviceCreateInfo deviceCI {};
        deviceCI.setQueueCreateInfos(queueCI);
        deviceCI.setPEnabledFeatures(&features);
        deviceCI.setPNext(&dynamicRendering);
        deviceCI.setPEnabledExtensionNames(requiredExtensions);

        mDevice = mPhysicalDevice.createDevice(deviceCI);

        if (mDevice != VK_NULL_HANDLE) return true;
        else return false;
        
        mDynamicLoader.init(mInstance, mDevice);

        vk::DebugUtilsMessengerCreateInfoEXT debugCI {};
        debugCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        debugCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugCI.setPfnUserCallback(ValidationLayerCallback);
        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugCI, nullptr, mDynamicLoader);
    }

    bool RHIDeviceVK::CreateVmaAllocator()
    {
        VmaVulkanFunctions functions = {};
        functions.vkGetInstanceProcAddr = mDynamicLoader.vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = mDynamicLoader.vkGetDeviceProcAddr;
        
        VmaAllocatorCreateInfo allocatorCI {};
        allocatorCI.physicalDevice = mPhysicalDevice;
        allocatorCI.device = mDevice;
        allocatorCI.instance = mInstance;
        allocatorCI.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCI.preferredLargeHeapBlockSize = 0;
        allocatorCI.pVulkanFunctions = &functions;

        return vmaCreateAllocator(&allocatorCI, &mAllocator) == VK_SUCCESS;
    }

    void RHIDeviceVK::FindQueueFamilyIndex()
    {
        auto queueFamilyProps = mPhysicalDevice.getQueueFamilyProperties();

        for (uint32_t i = 0; i < queueFamilyProps.size(); i++)
        {
            if (mGraphicsQueueIndex == uint32_t(-1))
            {
                if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics)
                {
                    mGraphicsQueueIndex = i;
                    continue;
                }
            }
            if (mCopyQueueIndex == uint32_t(-1))
            {
                if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eTransfer &&
                    !(queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                    !(queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eCompute))
                {
                    mCopyQueueIndex = i;
                    continue;
                }
            }
            if (mComputeQueueIndex == uint32_t(-1))
            {
                if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eCompute &&
                    !(queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    mComputeQueueIndex = i;
                    continue;
                }
            }
        }
        assert(mGraphicsQueueIndex != uint32_t(-1));
        assert(mCopyQueueIndex != uint32_t(-1));
        assert(mComputeQueueIndex != uint32_t(-1));
    }
}