#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    struct BufferCreateInfo;
    struct TextureCreateInfo;
    struct SamplerCreateInfo;
    struct BindGroupCreateInfo;
    struct BindGroupLayoutCreateInfo;
    struct PipelineLayoutCreateInfo;
    struct ShaderModuleCreateInfo;
    struct GraphicsPipelineCreateInfo;
    struct ComputePipelineCreateInfo;
    struct SwapchainCreateInfo;
    struct SurfaceCreateInfo;
    class RHIQueue;
    class RHIBuffer;
    class RHITexture;
    class RHISampler;
    class RHIBindGroup;
    class RHIBindGroupLayout;
    class RHIPipelineLayout;
    class RHIShaderModule;
    class RHIGraphicsPipeline;
    class RHIComputePipeline;
    class RHICommandBuffer;
    class RHISwapchain;
    class RHISurface;
    class RHIFence;

    struct QueueInfo
    {
        RHICommandQueueType Type;
        uint8_t Count;
    };

    struct DeviceCreateInfo
    {
        uint32_t QueueCreateInfoCount;
        const QueueInfo* QueueCreateInfos;
    };

    class RHIDevice
    {
    public:
        NOCOPY(RHIDevice)
        virtual ~RHIDevice() = default;
        virtual void Destroy() = 0;

        virtual size_t GetQueueCount(RHICommandQueueType type) = 0;
        virtual RHIQueue* GetQueue(RHICommandQueueType type, size_t index) = 0;
        virtual RHISurface* CreateSurface(const SurfaceCreateInfo& createInfo) = 0;
        virtual RHISwapchain* CreateSwapchain(const SwapchainCreateInfo& createInfo) = 0;
        virtual RHIBuffer* CreateBuffer(const BufferCreateInfo& createInfo) = 0;
        virtual RHITexture* CreateTexture(const TextureCreateInfo& createInfo) = 0;
        virtual RHISampler* CreateSampler(const SamplerCreateInfo& createInfo) = 0;
        virtual RHIBindGroup* CreateBindGroup(const BindGroupCreateInfo& createInfo) = 0;
        virtual RHIBindGroupLayout* CreateBindGroupLayout(const BindGroupLayoutCreateInfo& createInfo) = 0;
        virtual RHIPipelineLayout* CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo) = 0;
        virtual RHIShaderModule* CreateShaderModule(const ShaderModuleCreateInfo& createInfo) = 0;
        virtual RHIGraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) = 0;
        virtual RHIComputePipeline* CreateComputePipeline(const ComputePipelineCreateInfo& createInfo) = 0;
        virtual RHICommandBuffer* CreateCommandBuffer() = 0;
        virtual RHIFence* CreateFence(bool signaled) = 0;

        virtual bool CheckSwapchainFormatSupport(RHISurface* surface, RHIFormat format) = 0;

    protected:
        explicit RHIDevice(const DeviceCreateInfo& createInfo) {}
    };
} // namespace Vultana::RHI
