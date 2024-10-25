#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIBuffer;
    class RHITexture;
    class RHIFence;
    class RHIHeap;
    class RHIDescriptor;
    class RHIPipelineState;
    class RHISwapchain;
    
    class RHICommandList : public RHIResource
    {
    public:
        virtual ~RHICommandList() = default;

        ERHICommandQueueType GetQueueType() const { return mCmdQueueType; }

        virtual void ResetAllocator() = 0;
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void Wait(RHIFence* fence, uint64_t value) = 0;
        virtual void Signal(RHIFence* fence, uint64_t value) = 0;
        virtual void Present(RHISwapchain* swapchain) = 0;
        virtual void Submit() = 0;
        virtual void ResetState() = 0;

        virtual void BeginProfiling() = 0;
        virtual void EndProfiling() = 0;
        virtual void BeginEvent(const eastl::string& eventName) = 0;
        virtual void EndEvent() = 0;

        virtual void CopyBufferToTexture(RHIBuffer* srcBuffer, RHITexture* dstTexture, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset) = 0;
        virtual void CopyTextureToBuffer(RHITexture* srcTexture, RHIBuffer* dstBuffer, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset) = 0;
        virtual void CopyBuffer(RHIBuffer* src, RHIBuffer* dst, uint32_t srcOffset, uint32_t dstOffset, uint32_t size) = 0;
        virtual void CopyTexture(RHITexture* src, RHITexture* dst, uint32_t srcMipLevel, uint32_t dstMipLevel, uint32_t srcArraySlice, uint32_t dstArraySlice) = 0;
        virtual void ClearUAV(RHIResource* resource, RHIDescriptor* uav, const float* clearValue) = 0;
        virtual void ClearUAV(RHIResource* resource, RHIDescriptor* uav, const uint32_t* clearValue) = 0;
        virtual void WriteBuffer(RHIBuffer* buffer, uint32_t offset, uint32_t data) = 0;

        virtual void TextureBarrier(RHITexture* texture, uint32_t subResouce, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) = 0;
        virtual void BufferBarrier(RHIBuffer* buffer, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) = 0;
        virtual void GlobalBarrier(ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) = 0;
        virtual void FlushBarriers() = 0;

        virtual void BeginRenderPass(const RHIRenderPassDesc& desc) = 0;
        virtual void EndRenderPass() = 0;
        virtual void SetPipelineState(RHIPipelineState* pipelineState) = 0;
        virtual void SetStencilReference(uint8_t stencil) = 0;
        virtual void SetBlendFactor(const float* blendFactor) = 0;
        virtual void SetIndexBuffer(RHIBuffer* buffer, uint32_t offset, ERHIFormat format) = 0;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void SetGraphicsConstants(uint32_t slot, const void* data, size_t dataSize) = 0;
        virtual void SetComputeConstants(uint32_t slot, const void* data, size_t dataSize) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t indexOffset = 0) = 0;
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
        virtual void DispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

        virtual void DrawIndirect(RHIBuffer* buffer, uint32_t offset) = 0;
        virtual void DrawIndexedIndirect(RHIBuffer* buffer, uint32_t offset) = 0;
        virtual void DispatchIndirect(RHIBuffer* buffer, uint32_t offset) = 0;

    protected:
        ERHICommandQueueType mCmdQueueType;
    };
}