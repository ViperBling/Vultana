#pragma once

#include "Utilities/Math.hpp"

#include "RHICommon.hpp"

namespace RHI
{
    class RHIBuffer;
    class RHIBufferView;
    class RHITexture;
    class RHITextureView;
    class RHIComputePipeline;
    class RHIGraphicsPipeline;
    class RHIBindGroup;
    class RHISwapchain;
    struct RHIBarrier;

    struct TextureSubResourceCreateInfo
    {
        uint8_t MipLevel;
        uint8_t BaseArrayLayer;
        uint8_t LayerCount;
        Math::Vector3u Origin {};
        RHITextureType Type = RHITextureType::Color;
    };

    struct GraphicsPassColorAttachmentBase
    {
        Math::Vector4 ColorClearValue;
        RHILoadOp LoadOp;
        RHIStoreOp StoreOp;
    };

    struct GraphicsPassDepthStencilAttachmentBase
    {
        float DepthClearValue;
        RHILoadOp DepthLoadOp;
        RHIStoreOp DepthStoreOp;
        bool DepthReadOnly;
        uint32_t StencilClearValue;
        RHILoadOp StencilLoadOp;
        RHIStoreOp StencilStoreOp;
        bool StencilReadOnly;
    };

    struct GraphicsPassColorAttachment : public GraphicsPassColorAttachmentBase
    {
        RHITextureView* TextureView;
        RHITextureView* ResolveTextureView;
    };

    struct GraphicsPassDepthStencilAttachment : public GraphicsPassDepthStencilAttachmentBase
    {
        RHITextureView* TextureView;
    };

    struct GraphicsPassBeginInfo
    {
        uint32_t ColorAttachmentCount;
        const GraphicsPassColorAttachment* ColorAttachments;
        const GraphicsPassDepthStencilAttachment* DepthStencilAttachment;
    };

    class RHIComputePassCommandList
    {
    public:
        NOCOPY(RHIComputePassCommandList)
        virtual ~RHIComputePassCommandList() = default;

        virtual void SetPipeline(RHIComputePipeline* pipeline) = 0;
        virtual void SetBindGroup(uint32_t index, RHIBindGroup* bindGroup) = 0;
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
        virtual void EndPass() = 0;
        virtual void Destroy() = 0;

    protected:
        RHIComputePassCommandList() = default;
    };

    class RHIGraphicsPassCommandList
    {
    public:
        NOCOPY(RHIGraphicsPassCommandList)
        virtual ~RHIGraphicsPassCommandList() = default;

        virtual void SetPipeline(RHIGraphicsPipeline* pipeline) = 0;
        virtual void SetBindGroup(uint32_t index, RHIBindGroup* bindGroup) = 0;
        virtual void SetIndexBuffer(RHIBufferView* bufferView) = 0;
        virtual void SetVertexBuffers(uint32_t slot, RHIBufferView* bufferView) = 0;
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t firstInstance) = 0;
        virtual void SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth) = 0;
        virtual void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) = 0;
        virtual void SetPrimitiveTopology(RHIPrimitiveTopologyType topology) = 0;
        virtual void SetBlendConstants(const float blendConstants[4]) = 0;
        virtual void SetStencilReference(uint32_t reference) = 0;

        virtual void EndPass() = 0;
        virtual void Destroy() = 0;
    
    protected:
        RHIGraphicsPassCommandList() = default;
    };


    class RHICommandList
    {
    public:
        NOCOPY(RHICommandList)
        virtual ~RHICommandList() = default;

        virtual void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64_t srcOffset, uint64_t dstOffset, uint64_t size) = 0;
        virtual void CopyBufferToTexture(RHIBuffer* Src, RHITexture* Dst, const TextureSubResourceCreateInfo* subResourceInfo, const Math::Vector3u& size) = 0;
        virtual void CopyTextureToBuffer(RHITexture* Src, RHIBuffer* Dst, const TextureSubResourceCreateInfo* subResourceInfo, const Math::Vector3u& size) = 0;
        virtual void CopyTextureToTexture(RHITexture* Src, RHITexture* Dst, const TextureSubResourceCreateInfo* srcSubResourceInfo, const TextureSubResourceCreateInfo* dstSubResourceInfo, const Math::Vector3u& size) = 0;
        virtual void ResourceBarrier(const RHIBarrier& barrier) = 0;

        virtual RHIComputePassCommandList* BeginComputePass() = 0;
        virtual RHIGraphicsPassCommandList* BeginGraphicsPass(const GraphicsPassBeginInfo* info) = 0;
        virtual void SwapchainSync(RHISwapchain* swapchain) = 0;
        virtual void End() = 0;
        virtual void Destroy() = 0;

    protected:
        RHICommandList() = default;
    };
} // namespace Vultana
