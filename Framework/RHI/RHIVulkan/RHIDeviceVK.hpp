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
        virtual void* GetNativeHandle() const override { return mDevice; }

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

        vk::Instance GetInstance() const { return mInstance; }
        vk::DispatchLoaderDynamic GetDynamicLoader() const { return mDynamicLoader; }
        vk::PhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }
        vk::Device GetDevice() const { return mDevice; }
        VmaAllocator GetVmaAllocator() const { return mAllocator; }
        uint32_t GetGraphicsQueueIndex() const { return mGraphicsQueueIndex; }
        uint32_t GetComputeQueueIndex() const { return mComputeQueueIndex; }
        uint32_t GetCopyQueueIndex() const { return mCopyQueueIndex; }
        vk::Queue GetGraphicsQueue() const { return mGraphicsQueue; }
        vk::Queue GetComputeQueue() const { return mComputeQueue; }
        vk::Queue GetCopyQueue() const { return mCopyQueue; }
        vk::PipelineLayout GetPipelineLayout() const { return mPipelineLayout; }
        class RHIDescriptorAllocatorVK* GetResourceDescriptorAllocator() const { return mResourceDesAllocator; }
        class RHIDescriptorAllocatorVK* GetSamplerDescriptorAllocator() const { return mSamplerDesAllocator; }
        class RHIConstantBufferAllocatorVK* GetConstantBufferAllocator() const;
        const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& GetDescriptorBufferProperties() const { return mDescBufferProps; }

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
        vk::Instance mInstance = VK_NULL_HANDLE;
        vk::DebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
        vk::DispatchLoaderDynamic mDynamicLoader = {};
        vk::PhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        vk::Device mDevice = VK_NULL_HANDLE;
        VmaAllocator mAllocator = VK_NULL_HANDLE;
        vk::DescriptorSetLayout mDescSetLayout[3] = {};
        vk::PipelineLayout mPipelineLayout = {};
        vk::PhysicalDeviceDescriptorBufferPropertiesEXT mDescBufferProps = {};

        uint32_t mGraphicsQueueIndex = -1;
        uint32_t mComputeQueueIndex = -1;
        uint32_t mCopyQueueIndex = -1;
        vk::Queue mGraphicsQueue;
        vk::Queue mComputeQueue;
        vk::Queue mCopyQueue;

        RHIDeletionQueueVK* mDeferredDeletionQueue = nullptr;
        RHICommandList* mTransitionCopyCmdList[RHI_MAX_INFLIGHT_FRAMES] = {};
        RHICommandList* mTransitionGraphicsCmdList[RHI_MAX_INFLIGHT_FRAMES] = {};

        class RHIConstantBufferAllocatorVK* mConstantBufferAllocators[RHI_MAX_INFLIGHT_FRAMES] = {};
        class RHIDescriptorAllocatorVK* mResourceDesAllocator = nullptr;
        class RHIDescriptorAllocatorVK* mSamplerDesAllocator = nullptr;

        eastl::vector<eastl::pair<RHITexture*, ERHIAccessFlags>> mPendingGraphicsTransitions;
        eastl::vector<eastl::pair<RHITexture*, ERHIAccessFlags>> mPendingCopyTransitions;

        eastl::hash_map<RHITextureDesc, uint32_t> mTextureSizeMap;
    };

    template<typename T>
    inline void RHIDeviceVK::Delete(T objectHandle)
    {
        if (objectHandle != VK_NULL_HANDLE)
        {
            mDeferredDeletionQueue->Delete(objectHandle, mFrameID);
        }
    }
} // namespace RHI::Vulkan
