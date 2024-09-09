#pragma once

#include "RHICommonVK.hpp"
#include "RHIDeletionQueueVK.hpp"
#include "RHI/RHIDevice.hpp"

#include "Utilities/Hash.hpp"

#include <queue>
#include <unordered_map>

namespace std
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

        virtual RHISwapchain* CreateSwapchain(const RHISwapchainDesc& desc, const std::string& name) override;
        virtual RHICommandList* CreateCommandList(ERHICommandQueueType queueType, const std::string& name) override;
        virtual RHIFence* CreateFence(const std::string& name) override;
        virtual RHIHeap* CreateHeap(const RHIHeapDesc& desc, const std::string& name) override;
        virtual RHIBuffer* CreateBuffer(const RHIBufferDesc& desc, const std::string& name) override;
        virtual RHITexture* CreateTexture(const RHITextureDesc& desc, const std::string& name) override;
        virtual RHIShader* CreateShader(const RHIShaderDesc& desc, tcb::span<uint8_t> data, const std::string& name) override;
        virtual RHIPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, const std::string& name) override;
        virtual RHIPipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc, const std::string& name) override;
        virtual RHIDescriptor* CreateShaderResourceView(RHIResource* resource, const RHIShaderResourceViewDesc& desc, const std::string& name) override;
        virtual RHIDescriptor* CreateUnorderedAccessView(RHIResource* resource, const RHIUnorderedAccessViewDesc& desc, const std::string& name) override;
        virtual RHIDescriptor* CreateConstantBufferView(RHIResource* resource, const RHIConstantBufferViewDesc& desc, const std::string& name) override;
        virtual RHIDescriptor* CreateSampler(const RHISamplerDesc& desc, const std::string& name) override;

        virtual uint32_t GetAllocationSize(const RHIBufferDesc& desc) const override;
        virtual uint32_t GetAllocationSize(const RHITextureDesc& desc) const override;

        virtual bool DumpMemoryStats(const std::string& file) override;

        vk::Instance GetInstance() const { return mInstance; }
        vk::PhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }
        vk::Device GetDevice() const { return mDevice; }
        VmaAllocator GetVmaAllocator() const { return mAllocator; }
        uint32_t GetGraphicsQueueIndex() const { return mGraphicsQueueIndex; }
        uint32_t GetComputeQueueIndex() const { return mComputeQueueIndex; }
        uint32_t GetCopyQueueIndex() const { return mCopyQueueIndex; }
        vk::Queue GetGraphicsQueue() const { return mGraphicsQueue; }
        vk::Queue GetComputeQueue() const { return mComputeQueue; }
        vk::Queue GetCopyQueue() const { return mCopyQueue; }

        template<typename T>
        void Delete(T object);

        void EnqueueDefaultLayoutTransition(RHITexture* texture);
        void CancelDefaultLayoutTransition(RHITexture* texture);
        void FlushLayoutTransition(ERHICommandQueueType queueType);

    private:
        bool CreateInstance();
        bool CreateDevice();
        bool CreateVmaAllocator();
        void FindQueueFamilyIndex();

    private:
        vk::Instance mInstance;
        vk::DebugUtilsMessengerEXT mDebugMessenger;
        vk::DispatchLoaderDynamic mDynamicLoader;
        vk::PhysicalDevice mPhysicalDevice;
        vk::Device mDevice;
        VmaAllocator mAllocator = VK_NULL_HANDLE;

        uint32_t mGraphicsQueueIndex = -1;
        uint32_t mComputeQueueIndex = -1;
        uint32_t mCopyQueueIndex = -1;
        vk::Queue mGraphicsQueue;
        vk::Queue mComputeQueue;
        vk::Queue mCopyQueue;

        RHIDeletionQueueVK* mDeferredDeletionQueue = nullptr;
        RHICommandList* mTransitionCopyCmdList[RHI_MAX_INFLIGHT_FRAMES] = {};
        RHICommandList* mTransitionGraphicsCmdList[RHI_MAX_INFLIGHT_FRAMES] = {};

        std::vector<std::pair<RHITexture*, ERHIAccessFlags>> mPendingGraphicsTransitions;
        std::vector<std::pair<RHITexture*, ERHIAccessFlags>> mPendingCopyTransitions;

        std::unordered_map<RHITextureDesc, uint32_t> mTextureSizeMap;
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
