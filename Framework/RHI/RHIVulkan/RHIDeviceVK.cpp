#include "RHIDeviceVK.hpp"
#include "RHIBufferVK.hpp"
#include "RHICommandListVK.hpp"
#include "RHIDescriptorVK.hpp"
#include "RHIDescriptorAllocatorVK.hpp"
#include "RHIFenceVK.hpp"
#include "RHIHeapVK.hpp"
#include "RHIPipelineStateVK.hpp"
#include "RHIShaderVK.hpp"
#include "RHISwapchainVK.hpp"
#include "RHITextureVK.hpp"

#include "Utilities/Log.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

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
        mDevice.waitIdle();
        
        for (size_t i = 0; i < RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            delete mTransitionCopyCmdList[i];
            delete mTransitionGraphicsCmdList[i];
            delete mConstantBufferAllocators[i];
        }
        delete mDeferredDeletionQueue;

        delete mResourceDesAllocator;
        delete mSamplerDesAllocator;

        vmaDestroyAllocator(mAllocator);
        mDevice.destroyDescriptorSetLayout(mDescSetLayout[0]);
        mDevice.destroyDescriptorSetLayout(mDescSetLayout[1]);
        mDevice.destroyDescriptorSetLayout(mDescSetLayout[2]);
        mDevice.destroyPipelineLayout(mPipelineLayout);
        mDevice.destroy();
        mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger, nullptr, mDynamicLoader);
        mInstance.destroy();
    }

    bool RHIDeviceVK::Initialize()
    {
        CreateInstance();
        CreateDevice();
        CreateVmaAllocator();
        CreatePipelineLayout();

        mDeferredDeletionQueue = new RHIDeletionQueueVK(this);

        for (size_t i = 0; i < RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            mTransitionCopyCmdList[i] = CreateCommandList(ERHICommandQueueType::Copy, "Transition CmdList(Copy)");
            mTransitionGraphicsCmdList[i] = CreateCommandList(ERHICommandQueueType::Graphics, "Transition CmdList(Graphics)");

            mConstantBufferAllocators[i] = new RHIConstantBufferAllocatorVK(this, 8 * 1024 * 1024);
        }

        vk::PhysicalDeviceProperties2 props2 {};
        props2.pNext = &mDescBufferProps;
        mPhysicalDevice.getProperties2(&props2);

        size_t resourceDescSize = mDescBufferProps.sampledImageDescriptorSize;
        resourceDescSize = eastl::max(resourceDescSize, mDescBufferProps.storageImageDescriptorSize);
        resourceDescSize = eastl::max(resourceDescSize, mDescBufferProps.robustUniformTexelBufferDescriptorSize);
        resourceDescSize = eastl::max(resourceDescSize, mDescBufferProps.robustStorageTexelBufferDescriptorSize);
        resourceDescSize = eastl::max(resourceDescSize, mDescBufferProps.robustUniformBufferDescriptorSize);
        resourceDescSize = eastl::max(resourceDescSize, mDescBufferProps.robustStorageBufferDescriptorSize);
        // resourceDescSize = eastl::max(resourceDescSize, mDescBufferProps.accelerationStructureDescriptorSize);

        size_t samplerDescSize = mDescBufferProps.samplerDescriptorSize;

        mResourceDesAllocator = new RHIDescriptorAllocatorVK(this, (uint32_t)resourceDescSize, RHI_MAX_RESOURCE_DESCRIPTOR_COUNT, vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
        mSamplerDesAllocator = new RHIDescriptorAllocatorVK(this, (uint32_t)samplerDescSize, RHI_MAX_SAMPLER_DESCRIPTOR_COUNT, vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT);

        return true;
    }

    void RHIDeviceVK::BeginFrame()
    {
        mDeferredDeletionQueue->Flush();

        uint32_t index = mFrameID % RHI_MAX_INFLIGHT_FRAMES;
        mTransitionCopyCmdList[index]->ResetAllocator();
        mTransitionGraphicsCmdList[index]->ResetAllocator();
        mConstantBufferAllocators[index]->Reset();
    }

    void RHIDeviceVK::EndFrame()
    {
        ++mFrameID;

        vmaSetCurrentFrameIndex(mAllocator, (uint32_t)mFrameID);
    }

    RHISwapchain *RHIDeviceVK::CreateSwapchain(const RHISwapchainDesc &desc, const eastl::string &name)
    {
        RHISwapchainVK *swapchain = new RHISwapchainVK(this, desc, name);
        if (!swapchain->Create())
        {
            delete swapchain;
            return nullptr;
        }
        return swapchain;
    }

    RHICommandList *RHIDeviceVK::CreateCommandList(ERHICommandQueueType queueType, const eastl::string &name)
    {
        RHICommandListVK* cmdList = new RHICommandListVK(this, queueType, name);
        if (!cmdList->Create())
        {
            delete cmdList;
            return nullptr;
        }
        return cmdList;
    }

    RHIFence *RHIDeviceVK::CreateFence(const eastl::string &name)
    {
        RHIFenceVK* fence = new RHIFenceVK(this, name);
        if (!fence->Create())
        {
            delete fence;
            return nullptr;
        }
        return fence;
    }

    RHIHeap *RHIDeviceVK::CreateHeap(const RHIHeapDesc &desc, const eastl::string &name)
    {
        RHIHeapVK* heap = new RHIHeapVK(this, desc, name);
        if (!heap->Create())
        {
            delete heap;
            return nullptr;
        }
        return heap;
    }

    RHIBuffer *RHIDeviceVK::CreateBuffer(const RHIBufferDesc &desc, const eastl::string &name)
    {
        RHIBufferVK* buffer = new RHIBufferVK(this, desc, name);
        if (!buffer->Create())
        {
            delete buffer;
            return nullptr;
        }
        return buffer;
    }

    RHITexture *RHIDeviceVK::CreateTexture(const RHITextureDesc &desc, const eastl::string &name)
    {
        RHITextureVK* texture = new RHITextureVK(this, desc, name);
        if (!texture->Create())
        {
            delete texture;
            return nullptr;
        }
        return texture;
    }

    RHIShader *RHIDeviceVK::CreateShader(const RHIShaderDesc &desc, eastl::span<uint8_t> data, const eastl::string &name)
    {
        RHIShaderVK* shader = new RHIShaderVK(this, desc, name);
        if (!shader->Create(data))
        {
            delete shader;
            return nullptr;
        }
        return shader;
    }

    RHIPipelineState *RHIDeviceVK::CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc &desc, const eastl::string &name)
    {
        RHIGraphicsPipelineStateVK* pipeline = new RHIGraphicsPipelineStateVK(this, desc, name);
        if (!pipeline->Create())
        {
            delete pipeline;
            return nullptr;
        }
        return pipeline;
    }

    RHIPipelineState *RHIDeviceVK::CreateComputePipelineState(const RHIComputePipelineStateDesc &desc, const eastl::string &name)
    {
        RHIComputePipelineStateVK* pipeline = new RHIComputePipelineStateVK(this, desc, name);
        if (!pipeline->Create())
        {
            delete pipeline;
            return nullptr;
        }
        return pipeline;
    }

    RHIDescriptor *RHIDeviceVK::CreateShaderResourceView(RHIResource *resource, const RHIShaderResourceViewDesc &desc, const eastl::string &name)
    {
        RHIShaderResourceViewVK* srv = new RHIShaderResourceViewVK(this, resource, desc, name);
        if (!srv->Create())
        {
            delete srv;
            return nullptr;
        }
        return srv;
    }

    RHIDescriptor *RHIDeviceVK::CreateUnorderedAccessView(RHIResource *resource, const RHIUnorderedAccessViewDesc &desc, const eastl::string &name)
    {
        RHIUnorderedAccessViewVK* uav = new RHIUnorderedAccessViewVK(this, resource, desc, name);
        if (!uav->Create())
        {
            delete uav;
            return nullptr;
        }
        return uav;
    }

    RHIDescriptor *RHIDeviceVK::CreateConstantBufferView(RHIBuffer *resource, const RHIConstantBufferViewDesc &desc, const eastl::string &name)
    {
        RHIConstantBufferViewVK* cbv = new RHIConstantBufferViewVK(this, resource, desc, name);
        if (!cbv->Create())
        {
            delete cbv;
            return nullptr;
        }
        return cbv;
    }

    RHIDescriptor *RHIDeviceVK::CreateSampler(const RHISamplerDesc &desc, const eastl::string &name)
    {
        RHISamplerVK* sampler = new RHISamplerVK(this, desc, name);
        if (!sampler->Create())
        {
            delete sampler;
            return nullptr;
        }
        return sampler;
    }

    uint32_t RHIDeviceVK::GetAllocationSize(const RHIBufferDesc &desc)
    {
        return 0;
    }

    uint32_t RHIDeviceVK::GetAllocationSize(const RHITextureDesc &desc)
    {
        auto iter = mTextureSizeMap.find(desc);
        if (iter != mTextureSizeMap.end())
        {
            return iter->second;
        }

        vk::ImageCreateInfo imageCI = ToVulkanImageCreateInfo(desc);
        vk::Image image;
        image = mDevice.createImage(imageCI);
        if (!image)
        {
            return 0;
        }

        vk::ImageMemoryRequirementsInfo2 memReqInfo2 {};
        memReqInfo2.setImage(image);
        vk::MemoryRequirements2 requires2 = mDevice.getImageMemoryRequirements2(memReqInfo2);
        mDevice.destroyImage(image);

        mTextureSizeMap.emplace(desc, (uint32_t)requires2.memoryRequirements.size);
        return (uint32_t)requires2.memoryRequirements.size;
    }

    bool RHIDeviceVK::DumpMemoryStats(const eastl::string &file)
    {
        return false;
    }

    RHIConstantBufferAllocatorVK *RHIDeviceVK::GetConstantBufferAllocator() const
    {
        uint32_t index = mFrameID % RHI_MAX_INFLIGHT_FRAMES;
        return mConstantBufferAllocators[index];
    }

    uint32_t RHIDeviceVK::AllocateResourceDescriptor(void **desc)
    {
        return mResourceDesAllocator->Allocate(desc);
    }

    uint32_t RHIDeviceVK::AllocateSamplerDescriptor(void **desc)
    {
        return mSamplerDesAllocator->Allocate(desc);
    }

    void RHIDeviceVK::FreeResourceDescriptor(uint32_t index)
    {
        if (index != RHI_INVALID_RESOURCE)
        {
            mDeferredDeletionQueue->FreeResourceDescriptor(index, mFrameID);
        }
    }

    void RHIDeviceVK::FreeSamplerDescriptor(uint32_t index)
    {
        if (index != RHI_INVALID_RESOURCE)
        {
            mDeferredDeletionQueue->FreeSamplerDescriptor(index, mFrameID);
        }
    }

    vk::DeviceAddress RHIDeviceVK::AllocateConstantBuffer(const void *data, size_t dataSize)
    {
        void* cpuAddress = nullptr;
        vk::DeviceAddress gpuAddress = 0;
        GetConstantBufferAllocator()->Allocate((uint32_t)dataSize, &cpuAddress, &gpuAddress);

        memcpy(cpuAddress, data, dataSize);

        return gpuAddress;
    }

    vk::DeviceSize RHIDeviceVK::AllocateConstantBufferDescriptor(const uint32_t *cbv0, const vk::DescriptorAddressInfoEXT &cbv1, const vk::DescriptorAddressInfoEXT &cbv2)
    {
        size_t descBufferSize = sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS + mDescBufferProps.robustUniformBufferDescriptorSize * 2;
        void* cpuAddress = nullptr;
        vk::DeviceAddress gpuAddress = 0;
        GetConstantBufferAllocator()->Allocate((uint32_t)descBufferSize, &cpuAddress, &gpuAddress);

        memcpy(cpuAddress, cbv0, sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS);

        vk::DescriptorGetInfoEXT descGI {};
        descGI.type = vk::DescriptorType::eUniformBuffer;

        if (cbv1.address != 0)
        {
            descGI.data.pUniformBuffer = &cbv1;
            mDevice.getDescriptorEXT(descGI, mDescBufferProps.robustUniformBufferDescriptorSize, (char*)cpuAddress + sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS, mDynamicLoader);
        }

        if (cbv2.address != 0)
        {
            descGI.data.pUniformBuffer = &cbv2;
            mDevice.getDescriptorEXT(descGI, mDescBufferProps.robustUniformBufferDescriptorSize, (char*)cpuAddress + sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS + mDescBufferProps.robustUniformBufferDescriptorSize, mDynamicLoader);
        }

        vk::DeviceSize descBufferOffset = gpuAddress - GetConstantBufferAllocator()->GetGPUAddress();
        return descBufferOffset;
    }

    void RHIDeviceVK::EnqueueDefaultLayoutTransition(RHITexture *texture)
    {
        const RHITextureDesc& desc = texture->GetDesc();

        if (((RHITextureVK*)texture)->IsSwapchainTexture())
        {
            mPendingGraphicsTransitions.emplace_back(texture, RHIAccessPresent);
        }
        else if (desc.Usage & RHITextureUsageRenderTarget)
        {
            mPendingGraphicsTransitions.emplace_back(texture, RHIAccessRTV);
        }
        else if (desc.Usage & RHITextureUsageDepthStencil)
        {
            mPendingGraphicsTransitions.emplace_back(texture, RHIAccessDSV);
        }
        else if (desc.Usage & RHITextureUsageUnorderedAccess)
        {
            mPendingGraphicsTransitions.emplace_back(texture, RHIAccessMaskUAV);
        }
        else
        {
            mPendingCopyTransitions.emplace_back(texture, RHIAccessCopyDst);
        }
    }

    void RHIDeviceVK::CancelDefaultLayoutTransition(RHITexture *texture)
    {
        auto iter = eastl::find_if(mPendingGraphicsTransitions.begin(), mPendingGraphicsTransitions.end(),
        [texture](const eastl::pair<RHITexture*, ERHIAccessFlags>& transition)
        {
            return transition.first == texture;
        });
        if (iter != mPendingGraphicsTransitions.end())
        {
            mPendingGraphicsTransitions.erase(iter);
        }

        iter = eastl::find_if(mPendingCopyTransitions.begin(), mPendingCopyTransitions.end(),
        [texture](const eastl::pair<RHITexture*, ERHIAccessFlags>& transition)
        {
            return transition.first == texture;
        });
        if (iter != mPendingCopyTransitions.end())
        {
            mPendingCopyTransitions.erase(iter);
        }
    }

    void RHIDeviceVK::FlushLayoutTransition(ERHICommandQueueType queueType)
    {
        uint32_t index = mFrameID % RHI_MAX_INFLIGHT_FRAMES;

        if (queueType == ERHICommandQueueType::Graphics)
        {
            if (!mPendingGraphicsTransitions.empty() || !mPendingCopyTransitions.empty())
            {
                mTransitionGraphicsCmdList[index]->Begin();
                for (size_t i = 0; i < mPendingGraphicsTransitions.size(); i++)
                {
                    mTransitionGraphicsCmdList[index]->TextureBarrier(mPendingGraphicsTransitions[i].first, RHI_ALL_SUB_RESOURCE, RHIAccessDiscard, mPendingGraphicsTransitions[i].second);
                }
                mPendingGraphicsTransitions.clear();

                for (size_t i = 0; i < mPendingCopyTransitions.size(); i++)
                {
                    mTransitionGraphicsCmdList[index]->TextureBarrier(mPendingCopyTransitions[i].first, RHI_ALL_SUB_RESOURCE, RHIAccessDiscard, mPendingCopyTransitions[i].second);
                }
                mPendingCopyTransitions.clear();

                mTransitionGraphicsCmdList[index]->End();
                mTransitionGraphicsCmdList[index]->Submit();
            }
        }
        if (queueType == ERHICommandQueueType::Copy)
        {
            if (!mPendingCopyTransitions.empty())
            {
                mTransitionCopyCmdList[index]->Begin();
                for (size_t i = 0; i < mPendingCopyTransitions.size(); i++)
                {
                    mTransitionCopyCmdList[index]->TextureBarrier(mPendingCopyTransitions[i].first, RHI_ALL_SUB_RESOURCE, RHIAccessDiscard, mPendingCopyTransitions[i].second);
                }
                mPendingCopyTransitions.clear();

                mTransitionCopyCmdList[index]->End();
                mTransitionCopyCmdList[index]->Submit();
            }
        }
    }

    void RHIDeviceVK::CreateInstance()
    {
        auto supportExtens = vk::enumerateInstanceExtensionProperties();
        auto supportLayers = vk::enumerateInstanceLayerProperties();

        // VTNA_LOG_DEBUG("Available instance layers:");
        // for (uint32_t i = 0; i < supportLayers.size(); i++)
        // {
        //     VTNA_LOG_DEBUG("  {}", (char*)supportLayers[i].layerName);
        // }
        // VTNA_LOG_DEBUG("Available instance extensions:");
        // for (uint32_t i = 0; i < supportExtens.size(); i++)
        // {
        //     VTNA_LOG_DEBUG("  {}", (char*)supportExtens[i].extensionName);
        // }

        eastl::vector<const char*> requiredLayers = 
        {
            #if defined(_DEBUG) || defined(DEBUG)
            "VK_LAYER_KHRONOS_validation",
            #endif
        };

        eastl::vector<const char*> requiredExtensions = 
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
    }

    void RHIDeviceVK::CreateDevice()
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

        VTNA_LOG_INFO("GPU : {}", (char*)properties.deviceName);
        VTNA_LOG_INFO("API Version : {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));

        auto physicalDeviceExtenProps = mPhysicalDevice.enumerateDeviceExtensionProperties();

        // VTNA_LOG_DEBUG("Available device extensions:");
        // for (uint32_t i = 0; i < physicalDeviceExtenProps.size(); i++)
        // {
        //     VTNA_LOG_DEBUG("  {}", (char*)physicalDeviceExtenProps[i].extensionName);
        // }

        eastl::vector<const char*> requiredExtensions =
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
            // "VK_EXT_mesh_shader",
            "VK_EXT_descriptor_indexing",
            "VK_EXT_mutable_descriptor_type",
            "VK_EXT_descriptor_buffer",
            "VK_EXT_scalar_block_layout",
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

        vk::PhysicalDeviceVulkan12Features vk12Features {};
        vk12Features.setShaderInt8(VK_TRUE);
        vk12Features.setShaderFloat16(VK_TRUE);
        vk12Features.setDescriptorIndexing(VK_TRUE);
        vk12Features.setShaderUniformTexelBufferArrayDynamicIndexing(VK_TRUE);
        vk12Features.setShaderStorageTexelBufferArrayDynamicIndexing(VK_TRUE);
        vk12Features.setShaderUniformBufferArrayNonUniformIndexing(VK_TRUE);
        vk12Features.setShaderSampledImageArrayNonUniformIndexing(VK_TRUE);
        vk12Features.setShaderStorageBufferArrayNonUniformIndexing(VK_TRUE);
        vk12Features.setShaderStorageImageArrayNonUniformIndexing(VK_TRUE);
        vk12Features.setShaderUniformTexelBufferArrayNonUniformIndexing(VK_TRUE);
        vk12Features.setShaderStorageTexelBufferArrayNonUniformIndexing(VK_TRUE);
        vk12Features.setRuntimeDescriptorArray(VK_TRUE);
        vk12Features.setSamplerFilterMinmax(VK_TRUE);
        vk12Features.setScalarBlockLayout(VK_TRUE);
        vk12Features.setTimelineSemaphore(VK_TRUE);
        vk12Features.setBufferDeviceAddress(VK_TRUE);

        vk::PhysicalDeviceVulkan13Features vk13Features {};
        vk13Features.setPNext(&vk12Features);
        vk13Features.setInlineUniformBlock(VK_TRUE);
        vk13Features.setSynchronization2(VK_TRUE);
        vk13Features.setDynamicRendering(VK_TRUE);
        vk13Features.setSubgroupSizeControl(VK_TRUE);
        vk13Features.setShaderDemoteToHelperInvocation(VK_TRUE);

        vk::PhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDesc {};
        mutableDesc.setPNext(&vk13Features);
        mutableDesc.setMutableDescriptorType(VK_TRUE);

        vk::PhysicalDeviceDescriptorBufferFeaturesEXT descBuffer {};
        descBuffer.setPNext(&mutableDesc);
        descBuffer.setDescriptorBuffer(VK_TRUE);

        vk::PhysicalDeviceFeatures features {};
        features = mPhysicalDevice.getFeatures();

        vk::DeviceCreateInfo deviceCI {};
        deviceCI.setPNext(&descBuffer);
        deviceCI.setQueueCreateInfos(queueCI);
        deviceCI.setPEnabledFeatures(&features);
        deviceCI.setPEnabledExtensionNames(requiredExtensions);

        mDevice = mPhysicalDevice.createDevice(deviceCI);
        
        mDynamicLoader.init(mInstance, mDevice);

        vk::DebugUtilsMessengerCreateInfoEXT debugCI {};
        debugCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        debugCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugCI.setPfnUserCallback(ValidationLayerCallback);
        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugCI, nullptr, mDynamicLoader);

        mCopyQueue = mDevice.getQueue(mCopyQueueIndex, 0);
        mGraphicsQueue = mDevice.getQueue(mGraphicsQueueIndex, 0);
        mComputeQueue = mDevice.getQueue(mComputeQueueIndex, 0);
    }

    vk::Result RHIDeviceVK::CreateVmaAllocator()
    {
        VmaVulkanFunctions functions = {};
        functions.vkGetInstanceProcAddr = mDynamicLoader.vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = mDynamicLoader.vkGetDeviceProcAddr;
        
        VmaAllocatorCreateInfo allocatorCI {};
        allocatorCI.flags = VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocatorCI.physicalDevice = mPhysicalDevice;
        allocatorCI.device = mDevice;
        allocatorCI.instance = mInstance;
        allocatorCI.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCI.preferredLargeHeapBlockSize = 0;
        allocatorCI.pVulkanFunctions = &functions;

        return (vk::Result)vmaCreateAllocator(&allocatorCI, &mAllocator);
    }

    void RHIDeviceVK::CreatePipelineLayout()
    {
        eastl::vector<vk::DescriptorType> mutableDescTypes =
        {
            vk::DescriptorType::eSampledImage,
            vk::DescriptorType::eStorageImage,
            vk::DescriptorType::eUniformTexelBuffer,
            vk::DescriptorType::eStorageTexelBuffer,
            vk::DescriptorType::eUniformBuffer,
            vk::DescriptorType::eStorageBuffer,
            // vk::DescriptorType::eMutableEXT,
            // vk::DescriptorType::eAccelerationStructureKHR,
        };

        vk::MutableDescriptorTypeListEXT mutableDescList {};
        mutableDescList.setDescriptorTypes(mutableDescTypes);

        vk::MutableDescriptorTypeCreateInfoEXT mutableDescCI {};
        mutableDescCI.setMutableDescriptorTypeLists(mutableDescList);

        vk::DescriptorSetLayoutBinding constantBufferBinding[RHI_MAX_CBV_BINDING] = {};
        constantBufferBinding[0].setDescriptorType(vk::DescriptorType::eInlineUniformBlock);
        constantBufferBinding[0].setDescriptorCount(sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS);
        constantBufferBinding[0].setStageFlags(vk::ShaderStageFlagBits::eAll);

        for (size_t i = 1; i < RHI_MAX_CBV_BINDING; i++)
        {
            constantBufferBinding[i].setBinding((uint32_t)i);
            constantBufferBinding[i].setDescriptorType(vk::DescriptorType::eUniformBuffer);
            constantBufferBinding[i].setDescriptorCount(1);
            constantBufferBinding[i].setStageFlags(vk::ShaderStageFlagBits::eAll);
        }
        
        vk::DescriptorSetLayoutBinding resourceDescHeap {};
        resourceDescHeap.descriptorType = vk::DescriptorType::eMutableEXT;
        resourceDescHeap.descriptorCount = RHI_MAX_RESOURCE_DESCRIPTOR_COUNT;
        resourceDescHeap.stageFlags = vk::ShaderStageFlagBits::eAll;

        vk::DescriptorSetLayoutBinding samplerDescHeap {};
        samplerDescHeap.descriptorType = vk::DescriptorType::eSampler;
        samplerDescHeap.descriptorCount = RHI_MAX_SAMPLER_DESCRIPTOR_COUNT;
        samplerDescHeap.stageFlags = vk::ShaderStageFlagBits::eAll;

        // constant buffer
        vk::DescriptorSetLayoutCreateInfo descSetLayoutCI0 = {};
        descSetLayoutCI0.flags = vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT;
        descSetLayoutCI0.bindingCount = RHI_MAX_CBV_BINDING;
        descSetLayoutCI0.pBindings = constantBufferBinding;

        // resource descriptor heap
        vk::DescriptorSetLayoutCreateInfo descSetLayoutCI1 = {};
        descSetLayoutCI1.pNext = &mutableDescCI;
        descSetLayoutCI1.flags = vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT;
        descSetLayoutCI1.bindingCount = 1;
        descSetLayoutCI1.pBindings = &resourceDescHeap;

        // sampler descriptor heap
        vk::DescriptorSetLayoutCreateInfo descSetLayoutCI2 = {};
        descSetLayoutCI2.flags = vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT;
        descSetLayoutCI2.bindingCount = 1;
        descSetLayoutCI2.pBindings = &samplerDescHeap;

        mDescSetLayout[0] = mDevice.createDescriptorSetLayout(descSetLayoutCI0);
        mDescSetLayout[1] = mDevice.createDescriptorSetLayout(descSetLayoutCI1);
        mDescSetLayout[2] = mDevice.createDescriptorSetLayout(descSetLayoutCI2);

        vk::PipelineLayoutCreateInfo pipelineLayoutCI {};
        pipelineLayoutCI.setSetLayouts(mDescSetLayout);

        mPipelineLayout = mDevice.createPipelineLayout(pipelineLayoutCI);
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