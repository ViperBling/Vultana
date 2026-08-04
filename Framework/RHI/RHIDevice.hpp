#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    class FRHIResource;
    class FRHIBuffer;
    class FRHITexture;
    class FRHIFence;
    class FRHISwapchain;
    class FRHICommandList;
    class FRHIShader;
    class FRHIPipelineState;
    class FRHIDescriptor;
    class FRHIHeap;

    class FRHIDevice
    {
    public:
        virtual ~FRHIDevice() = default;

        const FRHIDeviceDesc& GetDesc() const { return m_Desc; }
        uint64_t GetFrameID() const { return m_FrameID; }

        virtual bool Initialize() = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void* GetNativeHandle() const = 0;

        virtual FRHISwapchain* CreateSwapchain(const FRHISwapchainDesc& desc, const eastl::string& name) = 0;
        virtual FRHICommandList* CreateCommandList(ERHICommandQueueType queueType, const eastl::string& name) = 0;
        virtual FRHIFence* CreateFence(const eastl::string& name) = 0;
        virtual FRHIHeap* CreateHeap(const FRHIHeapDesc& desc, const eastl::string& name) = 0;
        virtual FRHIBuffer* CreateBuffer(const FRHIBufferDesc& desc, const eastl::string& name) = 0;
        virtual FRHITexture* CreateTexture(const FRHITextureDesc& desc, const eastl::string& name) = 0;
        virtual FRHIShader* CreateShader(const FRHIShaderDesc& desc, eastl::span<uint8_t> data, const eastl::string& name) = 0;
        virtual FRHIPipelineState* CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& desc, const eastl::string& name) = 0;
        virtual FRHIPipelineState* CreateMeshShadingPipelineState(const FRHIMeshShadingPipelineStateDesc& desc, const eastl::string& name) = 0;
        virtual FRHIPipelineState* CreateComputePipelineState(const FRHIComputePipelineStateDesc& desc, const eastl::string& name) = 0;
        virtual FRHIDescriptor* CreateShaderResourceView(FRHIResource* resource, const FRHIShaderResourceViewDesc& desc, const eastl::string& name) = 0;
        virtual FRHIDescriptor* CreateUnorderedAccessView(FRHIResource* resource, const FRHIUnorderedAccessViewDesc& desc, const eastl::string& name) = 0;
        virtual FRHIDescriptor* CreateConstantBufferView(FRHIBuffer* resource, const FRHIConstantBufferViewDesc& desc, const eastl::string& name) = 0;
        virtual FRHIDescriptor* CreateSampler(const FRHISamplerDesc& desc, const eastl::string& name) = 0;

        virtual uint32_t GetAllocationSize(const FRHIBufferDesc& desc) = 0;
        virtual uint32_t GetAllocationSize(const FRHITextureDesc& desc) = 0;

        virtual bool DumpMemoryStats(const eastl::string& filename) = 0;

    protected:
        FRHIDeviceDesc m_Desc;
        uint64_t m_FrameID = 0;
    };
}