#pragma once

#include "RHI/RHIDevice.hpp"

#include <vulkan/vulkan.hpp>

namespace RHI::Vulkan
{
    class RHIDeviceVK : public RHIDevice
    {
    public:
        RHIDeviceVK(const RHIDeviceDesc& desc) : mDesc(desc) {}
        ~RHIDeviceVK();

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
        virtual RHIShader* CreateShader(const RHIShaderDesc& desc, std::span<uint8_t> data, const std::string& name) override;
        virtual RHIPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, const std::string& name) override;
        virtual RHIPipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc, const std::string& name) override;
        virtual RHIDescriptor* CreateShaderResourceView(RHIResource* resource, const RHIShaderResourceViewDesc& desc, const std::string& name) override;
        virtual RHIDescriptor* CreateUnorderedAccessView(RHIResource* resource, const RHIUnorderedAccessViewDesc& desc, const std::string& name) override;
        virtual RHIDescriptor* CreateConstantBufferView(RHIResource* resource, const RHIConstantBufferViewDesc& desc, const std::string& name) override;
        virtual RHIDescriptor* CreateSampler(const RHISamplerDesc& desc, const std::string& name) override;

        virtual uint32_t GetAllocationSize(const RHIBufferDesc& desc) const override;
        virtual uint32_t GetAllocationSize(const RHITextureDesc& desc) const override;

    private:
        RHIDeviceDesc mDesc {};

        vk::Device mDevice;
    };
} // namespace RHI::Vulkan
