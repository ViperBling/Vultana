#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIDevice.hpp"
#include "VulkanInstance.hpp"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <queue>

namespace RHI::Vulkan
{
    class RHIDeviceVK : public RHIDevice
    {
    public:
        RHIDeviceVK(const RHIDeviceDesc& desc) : mDesc(desc) {}
        ~RHIDeviceVK();

        virtual bool Initialize() override;
        virtual void BeginFrame() override;
        virtual void EndFrame() override;
        virtual uint64_t GetFrameID() const override;
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

        // void Delete()

    private:
        RHIDeviceDesc mDesc {};

        vk::Device mDevice;
        InstanceVK mInstance;

        VmaAllocator mAllocator;

        std::unique_ptr<vk::Queue> mGraphicsQueue;
        std::unique_ptr<vk::Queue> mComputeQueue;
        std::unique_ptr<vk::Queue> mTransferQueue;
    };
} // namespace RHI::Vulkan
