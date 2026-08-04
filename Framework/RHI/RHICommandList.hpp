#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class FRHIBuffer;
    class FRHITexture;
    class FRHIFence;
    class FRHIHeap;
    class FRHIDescriptor;
    class FRHIPipelineState;
    class FRHISwapchain;
    
    class FRHICommandList : public FRHIResource
    {
    public:
        virtual ~FRHICommandList() = default;

        ERHICommandQueueType GetQueueType() const { return m_CmdQueueType; }

        virtual void ResetAllocator() = 0;
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void Wait(FRHIFence* fence, uint64_t value) = 0;
        virtual void Signal(FRHIFence* fence, uint64_t value) = 0;
        virtual void Present(FRHISwapchain* swapchain) = 0;
        virtual void Submit() = 0;
        virtual void ResetState() = 0;

        virtual void BeginProfiling() = 0;
        virtual void EndProfiling() = 0;
        virtual void BeginEvent(const eastl::string& eventName) = 0;
        virtual void EndEvent() = 0;

        virtual void CopyBufferToTexture(FRHIBuffer* srcBuffer, FRHITexture* dstTexture, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset) = 0;
        virtual void CopyTextureToBuffer(FRHITexture* srcTexture, FRHIBuffer* dstBuffer, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset) = 0;
        virtual void CopyBuffer(FRHIBuffer* src, FRHIBuffer* dst, uint32_t srcOffset, uint32_t dstOffset, uint32_t size) = 0;
        virtual void CopyTexture(FRHITexture* src, FRHITexture* dst, uint32_t srcMipLevel, uint32_t dstMipLevel, uint32_t srcArraySlice, uint32_t dstArraySlice) = 0;
        virtual void ClearUAV(FRHIResource* resource, FRHIDescriptor* uav, const float* clearValue) = 0;
        virtual void ClearUAV(FRHIResource* resource, FRHIDescriptor* uav, const uint32_t* clearValue) = 0;
        virtual void WriteBuffer(FRHIBuffer* buffer, uint32_t offset, uint32_t data) = 0;

        virtual void TextureBarrier(FRHITexture* texture, uint32_t subResouce, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) = 0;
        virtual void BufferBarrier(FRHIBuffer* buffer, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) = 0;
        virtual void GlobalBarrier(ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) = 0;
        virtual void FlushBarriers() = 0;

        virtual void BeginRenderPass(const FRHIRenderPassDesc& desc) = 0;
        virtual void EndRenderPass() = 0;
        virtual void SetPipelineState(FRHIPipelineState* pipelineState) = 0;
        virtual void SetStencilReference(uint8_t stencil) = 0;
        virtual void SetBlendFactor(const float* blendFactor) = 0;
        virtual void SetIndexBuffer(FRHIBuffer* buffer, uint32_t offset, ERHIFormat format) = 0;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void SetGraphicsConstants(uint32_t slot, const void* data, size_t dataSize) = 0;
        virtual void SetComputeConstants(uint32_t slot, const void* data, size_t dataSize) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t indexOffset = 0) = 0;
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
        virtual void DispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

        virtual void DrawIndirect(FRHIBuffer* buffer, uint32_t offset) = 0;
        virtual void DrawIndexedIndirect(FRHIBuffer* buffer, uint32_t offset) = 0;
        virtual void DispatchIndirect(FRHIBuffer* buffer, uint32_t offset) = 0;
        virtual void DispatchMeshIndirect(FRHIBuffer* buffer, uint32_t offset) = 0;

        virtual void MultiDrawIndirect(uint32_t maxCount, FRHIBuffer* argsBuffer, uint32_t argsBufferOffset, FRHIBuffer* countBuffer, uint32_t countBufferOffset) = 0;
        virtual void MultiDrawIndexedIndirect(uint32_t maxCount, FRHIBuffer* argsBuffer, uint32_t argsBufferOffset, FRHIBuffer* countBuffer, uint32_t countBufferOffset) = 0;
        virtual void MultiDispatchIndirect(uint32_t maxCount, FRHIBuffer* argsBuffer, uint32_t argsBufferOffset, FRHIBuffer* countBuffer, uint32_t countBufferOffset) = 0;
        virtual void MultiDispatchMeshIndirect(uint32_t maxCount, FRHIBuffer* argsBuffer, uint32_t argsBufferOffset, FRHIBuffer* countBuffer, uint32_t countBufferOffset) = 0;

    protected:
        ERHICommandQueueType m_CmdQueueType;
    };
}