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
        m_Desc = desc;
    }

    RHIDeviceVK::~RHIDeviceVK()
    {
        m_Device.waitIdle();
        
        for (size_t i = 0; i < RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            delete m_TransitionCopyCmdList[i];
            delete m_TransitionGraphicsCmdList[i];
            delete m_ConstantBufferAllocators[i];
        }
        delete m_DeferredDeletionQueue;

        delete m_ResourceDesAllocator;
        delete m_SamplerDesAllocator;

        vmaDestroyAllocator(m_Allocator);
        m_Device.destroyDescriptorSetLayout(m_DescSetLayout[0]);
        m_Device.destroyDescriptorSetLayout(m_DescSetLayout[1]);
        m_Device.destroyDescriptorSetLayout(m_DescSetLayout[2]);
        m_Device.destroyPipelineLayout(m_PipelineLayout);
        m_Device.destroy();
        m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_DynamicLoader);
        m_Instance.destroy();
    }

    bool RHIDeviceVK::Initialize()
    {
        CreateInstance();
        CreateDevice();
        CreateVmaAllocator();
        CreatePipelineLayout();

        m_DeferredDeletionQueue = new RHIDeletionQueueVK(this);

        for (size_t i = 0; i < RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            m_TransitionCopyCmdList[i] = CreateCommandList(ERHICommandQueueType::Copy, "Transition CmdList(Copy)");
            m_TransitionGraphicsCmdList[i] = CreateCommandList(ERHICommandQueueType::Graphics, "Transition CmdList(Graphics)");

            m_ConstantBufferAllocators[i] = new RHIConstantBufferAllocatorVK(this, 8 * 1024 * 1024);
        }

        vk::PhysicalDeviceProperties2 props2 {};
        props2.pNext = &m_DescBufferProps;
        m_PhysicalDevice.getProperties2(&props2);

        size_t resourceDescSize = m_DescBufferProps.sampledImageDescriptorSize;
        resourceDescSize = eastl::max(resourceDescSize, m_DescBufferProps.storageImageDescriptorSize);
        resourceDescSize = eastl::max(resourceDescSize, m_DescBufferProps.robustUniformTexelBufferDescriptorSize);
        resourceDescSize = eastl::max(resourceDescSize, m_DescBufferProps.robustStorageTexelBufferDescriptorSize);
        resourceDescSize = eastl::max(resourceDescSize, m_DescBufferProps.robustUniformBufferDescriptorSize);
        resourceDescSize = eastl::max(resourceDescSize, m_DescBufferProps.robustStorageBufferDescriptorSize);
        // resourceDescSize = eastl::max(resourceDescSize, m_DescBufferProps.accelerationStructureDescriptorSize);

        size_t samplerDescSize = m_DescBufferProps.samplerDescriptorSize;

        m_ResourceDesAllocator = new RHIDescriptorAllocatorVK(this, (uint32_t)resourceDescSize, RHI_MAX_RESOURCE_DESCRIPTOR_COUNT, vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
        m_SamplerDesAllocator = new RHIDescriptorAllocatorVK(this, (uint32_t)samplerDescSize, RHI_MAX_SAMPLER_DESCRIPTOR_COUNT, vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT);

        return true;
    }

    void RHIDeviceVK::BeginFrame()
    {
        m_DeferredDeletionQueue->Flush();

        uint32_t index = m_FrameID % RHI_MAX_INFLIGHT_FRAMES;
        m_TransitionCopyCmdList[index]->ResetAllocator();
        m_TransitionGraphicsCmdList[index]->ResetAllocator();
        m_ConstantBufferAllocators[index]->Reset();
    }

    void RHIDeviceVK::EndFrame()
    {
        ++m_FrameID;

        vmaSetCurrentFrameIndex(m_Allocator, (uint32_t)m_FrameID);
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

    RHIPipelineState *RHIDeviceVK::CreateMeshShadingPipelineState(const RHIMeshShadingPipelineStateDesc &desc, const eastl::string &name)
    {
        RHIMeshShadingPipelineStateVK* pipeline = new RHIMeshShadingPipelineStateVK(this, desc, name);
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
        auto iter = m_TextureSizeMap.find(desc);
        if (iter != m_TextureSizeMap.end())
        {
            return iter->second;
        }

        vk::ImageCreateInfo imageCI = ToVulkanImageCreateInfo(desc);
        vk::Image image;
        image = m_Device.createImage(imageCI);
        if (!image)
        {
            return 0;
        }

        vk::ImageMemoryRequirementsInfo2 memReqInfo2 {};
        memReqInfo2.setImage(image);
        vk::MemoryRequirements2 requires2 = m_Device.getImageMemoryRequirements2(memReqInfo2);
        m_Device.destroyImage(image);

        m_TextureSizeMap.emplace(desc, (uint32_t)requires2.memoryRequirements.size);
        return (uint32_t)requires2.memoryRequirements.size;
    }

    bool RHIDeviceVK::DumpMemoryStats(const eastl::string &file)
    {
        return false;
    }

    RHIConstantBufferAllocatorVK *RHIDeviceVK::GetConstantBufferAllocator() const
    {
        uint32_t index = m_FrameID % RHI_MAX_INFLIGHT_FRAMES;
        return m_ConstantBufferAllocators[index];
    }

    uint32_t RHIDeviceVK::AllocateResourceDescriptor(void **desc)
    {
        return m_ResourceDesAllocator->Allocate(desc);
    }

    uint32_t RHIDeviceVK::AllocateSamplerDescriptor(void **desc)
    {
        return m_SamplerDesAllocator->Allocate(desc);
    }

    void RHIDeviceVK::FreeResourceDescriptor(uint32_t index)
    {
        if (index != RHI_INVALID_RESOURCE)
        {
            m_DeferredDeletionQueue->FreeResourceDescriptor(index, m_FrameID);
        }
    }

    void RHIDeviceVK::FreeSamplerDescriptor(uint32_t index)
    {
        if (index != RHI_INVALID_RESOURCE)
        {
            m_DeferredDeletionQueue->FreeSamplerDescriptor(index, m_FrameID);
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
        size_t descBufferSize = sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS + m_DescBufferProps.robustUniformBufferDescriptorSize * 2;
        void* cpuAddress = nullptr;
        vk::DeviceAddress gpuAddress = 0;
        GetConstantBufferAllocator()->Allocate((uint32_t)descBufferSize, &cpuAddress, &gpuAddress);

        memcpy(cpuAddress, cbv0, sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS);

        vk::DescriptorGetInfoEXT descGI {};
        descGI.type = vk::DescriptorType::eUniformBuffer;

        if (cbv1.address != 0)
        {
            descGI.data.pUniformBuffer = &cbv1;
            m_Device.getDescriptorEXT(descGI, m_DescBufferProps.robustUniformBufferDescriptorSize, (char*)cpuAddress + sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS, m_DynamicLoader);
        }

        if (cbv2.address != 0)
        {
            descGI.data.pUniformBuffer = &cbv2;
            m_Device.getDescriptorEXT(descGI, m_DescBufferProps.robustUniformBufferDescriptorSize, (char*)cpuAddress + sizeof(uint32_t) * RHI_MAX_ROOT_CONSTANTS + m_DescBufferProps.robustUniformBufferDescriptorSize, m_DynamicLoader);
        }

        vk::DeviceSize descBufferOffset = gpuAddress - GetConstantBufferAllocator()->GetGPUAddress();
        return descBufferOffset;
    }

    void RHIDeviceVK::EnqueueDefaultLayoutTransition(RHITexture *texture)
    {
        const RHITextureDesc& desc = texture->GetDesc();

        if (((RHITextureVK*)texture)->IsSwapchainTexture())
        {
            m_PendingGraphicsTransitions.emplace_back(texture, RHIAccessPresent);
        }
        else if (desc.Usage & RHITextureUsageRenderTarget)
        {
            m_PendingGraphicsTransitions.emplace_back(texture, RHIAccessRTV);
        }
        else if (desc.Usage & RHITextureUsageDepthStencil)
        {
            m_PendingGraphicsTransitions.emplace_back(texture, RHIAccessDSV);
        }
        else if (desc.Usage & RHITextureUsageUnorderedAccess)
        {
            m_PendingGraphicsTransitions.emplace_back(texture, RHIAccessMaskUAV);
        }
        else
        {
            m_PendingCopyTransitions.emplace_back(texture, RHIAccessCopyDst);
        }
    }

    void RHIDeviceVK::CancelDefaultLayoutTransition(RHITexture *texture)
    {
        auto iter = eastl::find_if(m_PendingGraphicsTransitions.begin(), m_PendingGraphicsTransitions.end(),
        [texture](const eastl::pair<RHITexture*, ERHIAccessFlags>& transition)
        {
            return transition.first == texture;
        });
        if (iter != m_PendingGraphicsTransitions.end())
        {
            m_PendingGraphicsTransitions.erase(iter);
        }

        iter = eastl::find_if(m_PendingCopyTransitions.begin(), m_PendingCopyTransitions.end(),
        [texture](const eastl::pair<RHITexture*, ERHIAccessFlags>& transition)
        {
            return transition.first == texture;
        });
        if (iter != m_PendingCopyTransitions.end())
        {
            m_PendingCopyTransitions.erase(iter);
        }
    }

    void RHIDeviceVK::FlushLayoutTransition(ERHICommandQueueType queueType)
    {
        uint32_t index = m_FrameID % RHI_MAX_INFLIGHT_FRAMES;

        if (queueType == ERHICommandQueueType::Graphics)
        {
            if (!m_PendingGraphicsTransitions.empty() || !m_PendingCopyTransitions.empty())
            {
                m_TransitionGraphicsCmdList[index]->Begin();
                for (size_t i = 0; i < m_PendingGraphicsTransitions.size(); i++)
                {
                    m_TransitionGraphicsCmdList[index]->TextureBarrier(m_PendingGraphicsTransitions[i].first, RHI_ALL_SUB_RESOURCE, RHIAccessDiscard, m_PendingGraphicsTransitions[i].second);
                }
                m_PendingGraphicsTransitions.clear();

                for (size_t i = 0; i < m_PendingCopyTransitions.size(); i++)
                {
                    m_TransitionGraphicsCmdList[index]->TextureBarrier(m_PendingCopyTransitions[i].first, RHI_ALL_SUB_RESOURCE, RHIAccessDiscard, m_PendingCopyTransitions[i].second);
                }
                m_PendingCopyTransitions.clear();

                m_TransitionGraphicsCmdList[index]->End();
                m_TransitionGraphicsCmdList[index]->Submit();
            }
        }
        if (queueType == ERHICommandQueueType::Copy)
        {
            if (!m_PendingCopyTransitions.empty())
            {
                m_TransitionCopyCmdList[index]->Begin();
                for (size_t i = 0; i < m_PendingCopyTransitions.size(); i++)
                {
                    m_TransitionCopyCmdList[index]->TextureBarrier(m_PendingCopyTransitions[i].first, RHI_ALL_SUB_RESOURCE, RHIAccessDiscard, m_PendingCopyTransitions[i].second);
                }
                m_PendingCopyTransitions.clear();

                m_TransitionCopyCmdList[index]->End();
                m_TransitionCopyCmdList[index]->Submit();
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

        m_Instance = vk::createInstance(instanceCI);
    }

    void RHIDeviceVK::CreateDevice()
    {
        auto physicalDevices = m_Instance.enumeratePhysicalDevices();
        for (const auto & pd : physicalDevices)
        {
            auto properties = pd.getProperties();
            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                m_PhysicalDevice = pd;
                break;
            }
        }
        auto properties = m_PhysicalDevice.getProperties();

        VTNA_LOG_INFO("GPU : {}", (char*)properties.deviceName);
        VTNA_LOG_INFO("API Version : {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));

        auto physicalDeviceExtenProps = m_PhysicalDevice.enumerateDeviceExtensionProperties();

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
            "VK_EXT_mesh_shader",
            "VK_EXT_descriptor_indexing",
            "VK_EXT_mutable_descriptor_type",
            "VK_EXT_descriptor_buffer",
            "VK_EXT_scalar_block_layout",
        };

        float queuePriorities[1] = {0.0};
        FindQueueFamilyIndex();

        vk::DeviceQueueCreateInfo queueCI[3] {};
        queueCI[0].setQueueFamilyIndex(m_CopyQueueIndex);
        queueCI[0].setQueueCount(1);
        queueCI[0].setQueuePriorities(queuePriorities);
        queueCI[1].setQueueFamilyIndex(m_GraphicsQueueIndex);
        queueCI[1].setQueueCount(1);
        queueCI[1].setQueuePriorities(queuePriorities);
        queueCI[2].setQueueFamilyIndex(m_ComputeQueueIndex);
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

        vk::PhysicalDeviceMeshShaderFeaturesEXT meshShader {};
        meshShader.setPNext(&vk13Features);
        meshShader.setTaskShader(VK_TRUE);
        meshShader.setMeshShader(VK_TRUE);

        vk::PhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDesc {};
        mutableDesc.setPNext(&meshShader);
        mutableDesc.setMutableDescriptorType(VK_TRUE);

        vk::PhysicalDeviceDescriptorBufferFeaturesEXT descBuffer {};
        descBuffer.setPNext(&mutableDesc);
        descBuffer.setDescriptorBuffer(VK_TRUE);

        vk::PhysicalDeviceFeatures features {};
        features = m_PhysicalDevice.getFeatures();

        vk::DeviceCreateInfo deviceCI {};
        deviceCI.setPNext(&descBuffer);
        deviceCI.setQueueCreateInfos(queueCI);
        deviceCI.setPEnabledFeatures(&features);
        deviceCI.setPEnabledExtensionNames(requiredExtensions);

        m_Device = m_PhysicalDevice.createDevice(deviceCI);
        
        m_DynamicLoader.init(m_Instance, m_Device);

        vk::DebugUtilsMessengerCreateInfoEXT debugCI {};
        debugCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        debugCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugCI.setPfnUserCallback(ValidationLayerCallback);
        m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugCI, nullptr, m_DynamicLoader);

        m_CopyQueue = m_Device.getQueue(m_CopyQueueIndex, 0);
        m_GraphicsQueue = m_Device.getQueue(m_GraphicsQueueIndex, 0);
        m_ComputeQueue = m_Device.getQueue(m_ComputeQueueIndex, 0);
    }

    vk::Result RHIDeviceVK::CreateVmaAllocator()
    {
        VmaVulkanFunctions functions = {};
        functions.vkGetInstanceProcAddr = m_DynamicLoader.vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = m_DynamicLoader.vkGetDeviceProcAddr;
        
        VmaAllocatorCreateInfo allocatorCI {};
        allocatorCI.flags = VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocatorCI.physicalDevice = m_PhysicalDevice;
        allocatorCI.device = m_Device;
        allocatorCI.instance = m_Instance;
        allocatorCI.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCI.preferredLargeHeapBlockSize = 0;
        allocatorCI.pVulkanFunctions = &functions;

        return (vk::Result)vmaCreateAllocator(&allocatorCI, &m_Allocator);
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

        m_DescSetLayout[0] = m_Device.createDescriptorSetLayout(descSetLayoutCI0);
        m_DescSetLayout[1] = m_Device.createDescriptorSetLayout(descSetLayoutCI1);
        m_DescSetLayout[2] = m_Device.createDescriptorSetLayout(descSetLayoutCI2);

        vk::PipelineLayoutCreateInfo pipelineLayoutCI {};
        pipelineLayoutCI.setSetLayouts(m_DescSetLayout);

        m_PipelineLayout = m_Device.createPipelineLayout(pipelineLayoutCI);
    }

    void RHIDeviceVK::FindQueueFamilyIndex()
    {
        auto queueFamilyProps = m_PhysicalDevice.getQueueFamilyProperties();

        for (uint32_t i = 0; i < queueFamilyProps.size(); i++)
        {
            if (m_GraphicsQueueIndex == uint32_t(-1))
            {
                if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics)
                {
                    m_GraphicsQueueIndex = i;
                    continue;
                }
            }
            if (m_CopyQueueIndex == uint32_t(-1))
            {
                if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eTransfer &&
                    !(queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                    !(queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eCompute))
                {
                    m_CopyQueueIndex = i;
                    continue;
                }
            }
            if (m_ComputeQueueIndex == uint32_t(-1))
            {
                if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eCompute &&
                    !(queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    m_ComputeQueueIndex = i;
                    continue;
                }
            }
        }
        assert(m_GraphicsQueueIndex != uint32_t(-1));
        assert(m_CopyQueueIndex != uint32_t(-1));
        assert(m_ComputeQueueIndex != uint32_t(-1));
    }
}