#pragma once

#include "RHICommonVK.hpp"
#include "RHIDeletionQueueVK.hpp"
#include "RHI/RHIDevice.hpp"

#include "Utilities/Hash.hpp"

#include <EASTL/hash_map.h>

namespace eastl
{
    template<>
    struct hash<RHI::FRHITextureDesc>
    {
        size_t operator()(const RHI::FRHITextureDesc& desc) const
        {
            return CityHash64(reinterpret_cast<const char*>(&desc), sizeof(desc));
        }
    };
}

namespace RHI
{
    class FVulkanDevice : public FRHIDevice
    {
    public:
        FVulkanDevice(const FRHIDeviceDesc& desc);
        ~FVulkanDevice();

        virtual bool Initialize() override;
        virtual void BeginFrame() override;
        virtual void EndFrame() override;
        virtual void* GetNativeHandle() const override { return m_Device; }

        virtual FRHISwapchain* CreateSwapchain(const FRHISwapchainDesc& desc, const eastl::string& name) override;
        virtual FRHICommandList* CreateCommandList(ERHICommandQueueType queueType, const eastl::string& name) override;
        virtual FRHIFence* CreateFence(const eastl::string& name) override;
        virtual FRHIHeap* CreateHeap(const FRHIHeapDesc& desc, const eastl::string& name) override;
        virtual FRHIBuffer* CreateBuffer(const FRHIBufferDesc& desc, const eastl::string& name) override;
        virtual FRHITexture* CreateTexture(const FRHITextureDesc& desc, const eastl::string& name) override;
        virtual FRHIShader* CreateShader(const FRHIShaderDesc& desc, eastl::span<uint8_t> data, const eastl::string& name) override;
        virtual FRHIPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& desc, const eastl::string& name) override;
        virtual FRHIPipelineState* CreateMeshShadingPipelineState(const FRHIMeshShadingPipelineStateDesc& desc, const eastl::string& name) override;
        virtual FRHIPipelineState* CreateComputePipelineState(const FRHIComputePipelineStateDesc& desc, const eastl::string& name) override;
        virtual FRHIDescriptor* CreateShaderResourceView(FRHIResource* resource, const FRHIShaderResourceViewDesc& desc, const eastl::string& name) override;
        virtual FRHIDescriptor* CreateUnorderedAccessView(FRHIResource* resource, const FRHIUnorderedAccessViewDesc& desc, const eastl::string& name) override;
        virtual FRHIDescriptor* CreateConstantBufferView(FRHIBuffer* resource, const FRHIConstantBufferViewDesc& desc, const eastl::string& name) override;
        virtual FRHIDescriptor* CreateSampler(const FRHISamplerDesc& desc, const eastl::string& name) override;

        virtual uint32_t GetAllocationSize(const FRHIBufferDesc& desc) override;
        virtual uint32_t GetAllocationSize(const FRHITextureDesc& desc) override;

        virtual bool DumpMemoryStats(const eastl::string& file) override;

        vk::Instance GetInstance() const { return m_Instance; }
        vk::detail::DispatchLoaderDynamic GetDynamicLoader() const { return m_DynamicLoader; }
        vk::PhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        vk::Device GetDevice() const { return m_Device; }
        VmaAllocator GetVmaAllocator() const { return m_Allocator; }
        uint32_t GetGraphicsQueueIndex() const { return m_GraphicsQueueIndex; }
        uint32_t GetComputeQueueIndex() const { return m_ComputeQueueIndex; }
        uint32_t GetCopyQueueIndex() const { return m_CopyQueueIndex; }
        vk::Queue GetGraphicsQueue() const { return m_GraphicsQueue; }
        vk::Queue GetComputeQueue() const { return m_ComputeQueue; }
        vk::Queue GetCopyQueue() const { return m_CopyQueue; }
        vk::PipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
        class FVulkanDescriptorAllocator* GetResourceDescriptorAllocator() const { return m_ResourceDesAllocator; }
        class FVulkanDescriptorAllocator* GetSamplerDescriptorAllocator() const { return m_SamplerDesAllocator; }
        class FVulkanConstantBufferAllocator* GetConstantBufferAllocator() const;
        const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& GetDescriptorBufferProperties() const { return m_DescBufferProps; }

        uint32_t AllocateResourceDescriptor(void** desc);
        uint32_t AllocateSamplerDescriptor(void** desc);
        void FreeResourceDescriptor(uint32_t index);
        void FreeSamplerDescriptor(uint32_t index);

        vk::DeviceAddress AllocateConstantBuffer(const void* data, size_t dataSize);
        vk::DeviceSize AllocateConstantBufferDescriptor(const uint32_t* cbv0, const vk::DescriptorAddressInfoEXT& cbv1, const vk::DescriptorAddressInfoEXT& cbv2);

        template<typename T>
        void Delete(T object);

        void EnqueueDefaultLayoutTransition(FRHITexture* texture);
        void CancelDefaultLayoutTransition(FRHITexture* texture);
        void FlushLayoutTransition(ERHICommandQueueType queueType);

    private:
        void CreateInstance();
        void CreateDevice();
        vk::Result CreateVmaAllocator();
        void CreatePipelineLayout();
        void FindQueueFamilyIndex();

    private:
        vk::Instance m_Instance = VK_NULL_HANDLE;
        vk::DebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        vk::detail::DispatchLoaderDynamic m_DynamicLoader = {};
        vk::PhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        vk::Device m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        vk::DescriptorSetLayout m_DescSetLayout[3] = {};
        vk::PipelineLayout m_PipelineLayout = {};
        vk::PhysicalDeviceDescriptorBufferPropertiesEXT m_DescBufferProps = {};

        uint32_t m_GraphicsQueueIndex = -1;
        uint32_t m_ComputeQueueIndex = -1;
        uint32_t m_CopyQueueIndex = -1;
        vk::Queue m_GraphicsQueue;
        vk::Queue m_ComputeQueue;
        vk::Queue m_CopyQueue;

        FVulkanDeletionQueue* m_DeferredDeletionQueue = nullptr;
        FRHICommandList* m_TransitionCopyCmdList[RHI_MAX_INFLIGHT_FRAMES] = {};
        FRHICommandList* m_TransitionGraphicsCmdList[RHI_MAX_INFLIGHT_FRAMES] = {};

        class FVulkanConstantBufferAllocator* m_ConstantBufferAllocators[RHI_MAX_INFLIGHT_FRAMES] = {};
        class FVulkanDescriptorAllocator* m_ResourceDesAllocator = nullptr;
        class FVulkanDescriptorAllocator* m_SamplerDesAllocator = nullptr;

        eastl::vector<eastl::pair<FRHITexture*, ERHIAccessFlags>> m_PendingGraphicsTransitions;
        eastl::vector<eastl::pair<FRHITexture*, ERHIAccessFlags>> m_PendingCopyTransitions;

        eastl::hash_map<FRHITextureDesc, uint32_t> m_TextureSizeMap;
    };

    template<typename T>
    inline void FVulkanDevice::Delete(T objectHandle)
    {
        if (objectHandle != VK_NULL_HANDLE)
        {
            m_DeferredDeletionQueue->Delete(objectHandle, m_FrameID);
        }
    }
} // namespace RHI::Vulkan
