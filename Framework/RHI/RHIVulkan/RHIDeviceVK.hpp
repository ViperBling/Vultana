#pragma once

#include "RHICommonVK.hpp"
#include "RHIDeletionQueueVK.hpp"
#include "RHI/RHIDevice.hpp"

#include "Utilities/Hash.hpp"

#include <EASTL/hash_map.h>

namespace eastl
{
    template<>
    struct hash<RHI::RHITextureDesc>
    {
        size_t operator()(const RHI::RHITextureDesc& desc) const
        {
            return CityHash64(reinterpret_cast<const char*>(&desc), sizeof(desc));
        }
    };
}

namespace RHI
{
    class RHIDeviceVK : public RHIDevice
    {
    public:
        RHIDeviceVK(const RHIDeviceDesc& desc);
        ~RHIDeviceVK();

        virtual bool Initialize() override;
        virtual void BeginFrame() override;
        virtual void EndFrame() override;
        virtual void* GetNativeHandle() const override { return m_Device; }

        virtual RHISwapchain* CreateSwapchain(const RHISwapchainDesc& desc, const eastl::string& name) override;
        virtual RHICommandList* CreateCommandList(ERHICommandQueueType queueType, const eastl::string& name) override;
        virtual RHIFence* CreateFence(const eastl::string& name) override;
        virtual RHIHeap* CreateHeap(const RHIHeapDesc& desc, const eastl::string& name) override;
        virtual RHIBuffer* CreateBuffer(const RHIBufferDesc& desc, const eastl::string& name) override;
        virtual RHITexture* CreateTexture(const RHITextureDesc& desc, const eastl::string& name) override;
        virtual RHIShader* CreateShader(const RHIShaderDesc& desc, eastl::span<uint8_t> data, const eastl::string& name) override;
        virtual RHIPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, const eastl::string& name) override;
        virtual RHIPipelineState* CreateMeshShadingPipelineState(const RHIMeshShadingPipelineStateDesc& desc, const eastl::string& name) override;
        virtual RHIPipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc, const eastl::string& name) override;
        virtual RHIDescriptor* CreateShaderResourceView(RHIResource* resource, const RHIShaderResourceViewDesc& desc, const eastl::string& name) override;
        virtual RHIDescriptor* CreateUnorderedAccessView(RHIResource* resource, const RHIUnorderedAccessViewDesc& desc, const eastl::string& name) override;
        virtual RHIDescriptor* CreateConstantBufferView(RHIBuffer* resource, const RHIConstantBufferViewDesc& desc, const eastl::string& name) override;
        virtual RHIDescriptor* CreateSampler(const RHISamplerDesc& desc, const eastl::string& name) override;

        virtual uint32_t GetAllocationSize(const RHIBufferDesc& desc) override;
        virtual uint32_t GetAllocationSize(const RHITextureDesc& desc) override;

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
        class RHIDescriptorAllocatorVK* GetResourceDescriptorAllocator() const { return m_ResourceDesAllocator; }
        class RHIDescriptorAllocatorVK* GetSamplerDescriptorAllocator() const { return m_SamplerDesAllocator; }
        class RHIConstantBufferAllocatorVK* GetConstantBufferAllocator() const;
        const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& GetDescriptorBufferProperties() const { return m_DescBufferProps; }

        uint32_t AllocateResourceDescriptor(void** desc);
        uint32_t AllocateSamplerDescriptor(void** desc);
        void FreeResourceDescriptor(uint32_t index);
        void FreeSamplerDescriptor(uint32_t index);

        vk::DeviceAddress AllocateConstantBuffer(const void* data, size_t dataSize);
        vk::DeviceSize AllocateConstantBufferDescriptor(const uint32_t* cbv0, const vk::DescriptorAddressInfoEXT& cbv1, const vk::DescriptorAddressInfoEXT& cbv2);

        template<typename T>
        void Delete(T object);

        void EnqueueDefaultLayoutTransition(RHITexture* texture);
        void CancelDefaultLayoutTransition(RHITexture* texture);
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

        RHIDeletionQueueVK* m_DeferredDeletionQueue = nullptr;
        RHICommandList* m_TransitionCopyCmdList[RHI_MAX_INFLIGHT_FRAMES] = {};
        RHICommandList* m_TransitionGraphicsCmdList[RHI_MAX_INFLIGHT_FRAMES] = {};

        class RHIConstantBufferAllocatorVK* m_ConstantBufferAllocators[RHI_MAX_INFLIGHT_FRAMES] = {};
        class RHIDescriptorAllocatorVK* m_ResourceDesAllocator = nullptr;
        class RHIDescriptorAllocatorVK* m_SamplerDesAllocator = nullptr;

        eastl::vector<eastl::pair<RHITexture*, ERHIAccessFlags>> m_PendingGraphicsTransitions;
        eastl::vector<eastl::pair<RHITexture*, ERHIAccessFlags>> m_PendingCopyTransitions;

        eastl::hash_map<RHITextureDesc, uint32_t> m_TextureSizeMap;
    };

    template<typename T>
    inline void RHIDeviceVK::Delete(T objectHandle)
    {
        if (objectHandle != VK_NULL_HANDLE)
        {
            m_DeferredDeletionQueue->Delete(objectHandle, m_FrameID);
        }
    }
} // namespace RHI::Vulkan
