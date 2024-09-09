#pragma once

#include "RHICommon.hpp"

// #include <tcb/span.hpp>
// #include <span>

namespace RHI
{
    class RHIResource;
    class RHIBuffer;
    class RHITexture;
    class RHIFence;
    class RHISwapchain;
    class RHICommandList;
    class RHIShader;
    class RHIPipelineState;
    class RHIDescriptor;
    class RHIHeap;

    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        const RHIDeviceDesc& GetDesc() const { return mDesc; }
        uint64_t GetFrameID() const { return mFrameID; }

        virtual bool Initialize() = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void* GetNativeHandle() const = 0;

        virtual RHISwapchain* CreateSwapchain(const RHISwapchainDesc& desc, const std::string& name) = 0;
        virtual RHICommandList* CreateCommandList(ERHICommandQueueType queueType, const std::string& name) = 0;
        virtual RHIFence* CreateFence(const std::string& name) = 0;
        virtual RHIHeap* CreateHeap(const RHIHeapDesc& desc, const std::string& name) = 0;
        virtual RHIBuffer* CreateBuffer(const RHIBufferDesc& desc, const std::string& name) = 0;
        virtual RHITexture* CreateTexture(const RHITextureDesc& desc, const std::string& name) = 0;
        virtual RHIShader* CreateShader(const RHIShaderDesc& desc, tcb::span<uint8_t> data, const std::string& name) = 0;
        virtual RHIPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, const std::string& name) = 0;
        virtual RHIPipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc, const std::string& name) = 0;
        virtual RHIDescriptor* CreateShaderResourceView(RHIResource* resource, const RHIShaderResourceViewDesc& desc, const std::string& name) = 0;
        virtual RHIDescriptor* CreateUnorderedAccessView(RHIResource* resource, const RHIUnorderedAccessViewDesc& desc, const std::string& name) = 0;
        virtual RHIDescriptor* CreateConstantBufferView(RHIResource* resource, const RHIConstantBufferViewDesc& desc, const std::string& name) = 0;
        virtual RHIDescriptor* CreateSampler(const RHISamplerDesc& desc, const std::string& name) = 0;

        virtual uint32_t GetAllocationSize(const RHIBufferDesc& desc) const = 0;
        virtual uint32_t GetAllocationSize(const RHITextureDesc& desc) const = 0;

        virtual bool DumpMemoryStats(const std::string& filename) = 0;

    protected:
        RHIDeviceDesc mDesc;
        uint64_t mFrameID = 0;
    };
}