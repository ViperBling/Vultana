#pragma once

#include "RHICommon.hpp"

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

        const RHIDeviceDesc& GetDesc() const { return m_Desc; }
        uint64_t GetFrameID() const { return m_FrameID; }

        virtual bool Initialize() = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void* GetNativeHandle() const = 0;

        virtual RHISwapchain* CreateSwapchain(const RHISwapchainDesc& desc, const eastl::string& name) = 0;
        virtual RHICommandList* CreateCommandList(ERHICommandQueueType queueType, const eastl::string& name) = 0;
        virtual RHIFence* CreateFence(const eastl::string& name) = 0;
        virtual RHIHeap* CreateHeap(const RHIHeapDesc& desc, const eastl::string& name) = 0;
        virtual RHIBuffer* CreateBuffer(const RHIBufferDesc& desc, const eastl::string& name) = 0;
        virtual RHITexture* CreateTexture(const RHITextureDesc& desc, const eastl::string& name) = 0;
        virtual RHIShader* CreateShader(const RHIShaderDesc& desc, eastl::span<uint8_t> data, const eastl::string& name) = 0;
        virtual RHIPipelineState* CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, const eastl::string& name) = 0;
        virtual RHIPipelineState* CreateMeshShadingPipelineState(const RHIMeshShadingPipelineStateDesc& desc, const eastl::string& name) = 0;
        virtual RHIPipelineState* CreateComputePipelineState(const RHIComputePipelineStateDesc& desc, const eastl::string& name) = 0;
        virtual RHIDescriptor* CreateShaderResourceView(RHIResource* resource, const RHIShaderResourceViewDesc& desc, const eastl::string& name) = 0;
        virtual RHIDescriptor* CreateUnorderedAccessView(RHIResource* resource, const RHIUnorderedAccessViewDesc& desc, const eastl::string& name) = 0;
        virtual RHIDescriptor* CreateConstantBufferView(RHIBuffer* resource, const RHIConstantBufferViewDesc& desc, const eastl::string& name) = 0;
        virtual RHIDescriptor* CreateSampler(const RHISamplerDesc& desc, const eastl::string& name) = 0;

        virtual uint32_t GetAllocationSize(const RHIBufferDesc& desc) = 0;
        virtual uint32_t GetAllocationSize(const RHITextureDesc& desc) = 0;

        virtual bool DumpMemoryStats(const eastl::string& filename) = 0;

    protected:
        RHIDeviceDesc m_Desc;
        uint64_t m_FrameID = 0;
    };
}