#include "DeviceVK.hpp"
#include "RHICommonVK.hpp"
#include "QueueVK.hpp"
#include "GPUVK.hpp"
#include "InstanceVK.hpp"

#include <map>

namespace Vultana
{
    const std::vector<const char *> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
    };

    const std::vector<const char *> DEVICE_LAYERS = {
        VK_KRONOS_VALIDATION_LAYER_NAME,
    };

    DeviceVK::DeviceVK(GPUVK &gpu, const DeviceCreateInfo &createInfo) 
        : RHIDevice(createInfo)
        , mGPU(gpu)
    {
        CreateDevice(createInfo);
        GetQueues();
        CreateVmaAllocator();
    }

    DeviceVK::~DeviceVK()
    {
        Destroy();
    }

    void DeviceVK::Destroy()
    {
        vmaDestroyAllocator(mAllocator);
        for (auto& [queue, pool] : mCommandPools)
        {
            mDevice.destroyCommandPool(pool);
        }
        mDevice.destroy();
    }

    size_t DeviceVK::GetQueueCount(RHICommandQueueType type)
    {
        auto it = mQueues.find(type);
        assert(it != mQueues.end());
        return it->second.size();
    }

    RHIQueue* DeviceVK::GetQueue(RHICommandQueueType type, size_t index)
    {
        auto it = mQueues.find(type);
        assert(it != mQueues.end());
        auto& queueArray = it->second;
        assert(index < queueArray.size());
        return queueArray[index].get();
    }

    RHISurface *DeviceVK::CreateSurface(const SurfaceCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHISwapchain *DeviceVK::CreateSwapchain(const SwapchainCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHIBuffer *DeviceVK::CreateBuffer(const BufferCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHITexture *DeviceVK::CreateTexture(const TextureCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHISampler *DeviceVK::CreateSampler(const SamplerCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHIBindGroup *DeviceVK::CreateBindGroup(const BindGroupCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHIBindGroupLayout *DeviceVK::CreateBindGroupLayout(const BindGroupLayoutCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHIPipelineLayout *DeviceVK::CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHIShaderModule *DeviceVK::CreateShaderModule(const ShaderModuleCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHIGraphicsPipeline *DeviceVK::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHIComputePipeline *DeviceVK::CreateComputePipeline(const ComputePipelineCreateInfo &createInfo)
    {
        return nullptr;
    }

    RHICommandBuffer *DeviceVK::CreateCommandBuffer()
    {
        return nullptr;
    }

    RHIFence *DeviceVK::CreateFence()
    {
        return nullptr;
    }

    bool DeviceVK::CheckSwapchainFormatSupport(RHISurface *surface, RHIFormat format)
    {
        return false;
    }

    void DeviceVK::SetObjectName(vk::ObjectType objectType, uint64_t handle, const std::string &name)
    {
        vk::DebugUtilsObjectNameInfoEXT info {};
        info.objectType = objectType;
        info.objectHandle = handle;
        info.pObjectName = name.c_str();

        mDevice.setDebugUtilsObjectNameEXT(info, mGPU.GetInstance().GetVkDynamicLoader());
    }

    std::optional<uint32_t> DeviceVK::FindQueueFamilyIndex(const std::vector<vk::QueueFamilyProperties> &properties, std::vector<uint32_t> usedQueueFamily, RHICommandQueueType type)
    {
        for (uint32_t i = 0; i < properties.size(); ++i)
        {
            auto it = std::find(usedQueueFamily.begin(), usedQueueFamily.end(), i);
            if (it != usedQueueFamily.end()) { continue; }
            if (properties[i].queueFlags & VKEnumCast<RHICommandQueueType, vk::QueueFlagBits>(type))
            {
                usedQueueFamily.emplace_back(i);
                return i;
            }
        }
        return {};
    }

    void DeviceVK::CreateDevice(const DeviceCreateInfo &createInfo)
    {
        auto queueFamilyProps = mGPU.GetVKPhysicalDevice().getQueueFamilyProperties();

        std::map<RHICommandQueueType, uint32_t> queueNumMap;
        for (size_t i = 0; i < createInfo.QueueCreateInfoCount; i++)
        {
            const auto& queueCI = createInfo.QueueCreateInfos[i];
            auto it = queueNumMap.find(queueCI.Type);
            if (it == queueNumMap.end()) { queueNumMap[queueCI.Type] = 0; }
            queueNumMap[queueCI.Type] += queueCI.Count;
        }

        std::vector<vk::DeviceQueueCreateInfo> queueInfos;
        std::vector<uint32_t> usedQueueFamily;
        std::vector<float> queueProperties;
        for (auto it : queueNumMap)
        {
            auto queueFamilyIndex = FindQueueFamilyIndex(queueFamilyProps, usedQueueFamily, it.first);
            assert(queueFamilyIndex.has_value());
            auto queueCount = std::min(queueFamilyProps[queueFamilyIndex.value()].queueCount, it.second);
            if (queueCount > queueProperties.size()) { queueProperties.resize(queueCount, 1.0f); }

            vk::DeviceQueueCreateInfo tempCI {};
            tempCI.queueFamilyIndex = queueFamilyIndex.value();
            tempCI.setQueuePriorities(queueProperties);
            queueInfos.emplace_back(tempCI);

            mQueueFamilyMappings[it.first] = std::make_pair(queueFamilyIndex.value(), queueCount);
        }

        // 由于使用了PipelineBarrier2，所以必须启用Sync2Feature
        vk::PhysicalDeviceVulkan12Features features12{};
        features12.setBufferDeviceAddress(true);
        features12.setDescriptorIndexing(true);
        vk::PhysicalDeviceVulkan13Features features13{};
        features13.setSynchronization2(true);
        features13.setDynamicRendering(true);
        features13.setPNext(&features12);

        vk::DeviceCreateInfo deviceCI {};
        deviceCI.setQueueCreateInfos(queueInfos);
        deviceCI.setPEnabledExtensionNames(DEVICE_EXTENSIONS);
        deviceCI.setPEnabledLayerNames(DEVICE_LAYERS);
        deviceCI.setPNext(&features13);
        GDebugInfoCallback("Created Vulkan Device", "VulkanDevice");

       vk::resultCheck(mGPU.GetVKPhysicalDevice().createDevice(&deviceCI, nullptr, &mDevice), nullptr);
    }

    void DeviceVK::GetQueues()
    {
        vk::CommandPoolCreateInfo cmdCI {};
        cmdCI.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

        for (auto it : mQueueFamilyMappings)
        {
            auto queueType = it.first;
            auto queueFamilyIndex = it.second.first;
            auto queueCount = it.second.second;

            std::vector<std::unique_ptr<QueueVK>> tempQueues(queueCount);
            for (auto i = 0; i < tempQueues.size(); i++)
            {
                vk::Queue queue = mDevice.getQueue(queueFamilyIndex, i, mGPU.GetInstance().GetVkDynamicLoader());
                tempQueues[i] = std::make_unique<QueueVK>(queue);
            }
            mQueues[queueType] = std::move(tempQueues);
            
            cmdCI.queueFamilyIndex = queueFamilyIndex;
            vk::CommandPool cmdPool;
            vk::resultCheck(mDevice.createCommandPool(&cmdCI, nullptr, &cmdPool), nullptr);
            mCommandPools.emplace(queueType, cmdPool);
        }

        GDebugInfoCallback("Created logical device and queue", "VulkanDevice");
    }

    void DeviceVK::CreateVmaAllocator()
    {
        VmaVulkanFunctions vkFuncs {};
        vkFuncs.vkGetInstanceProcAddr = mGPU.GetInstance().GetVkDynamicLoader().vkGetInstanceProcAddr;
        vkFuncs.vkGetDeviceProcAddr = mGPU.GetInstance().GetVkDynamicLoader().vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo vmaAI {};
        vmaAI.vulkanApiVersion = VK_API_VERSION_1_3;
        vmaAI.instance = mGPU.GetInstance().GetVkInstance();
        vmaAI.physicalDevice = mGPU.GetVKPhysicalDevice();
        vmaAI.device = mDevice;
        vmaAI.pVulkanFunctions = &vkFuncs;

        vmaCreateAllocator(&vmaAI, &mAllocator);
    }
}