#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHICommandList.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHICommandListVK : public RHICommandList
    {
    public:
        RHICommandListVK(RHIDeviceVK* device, ERHICommandQueueType queueType, const eastl::string& name);
        ~RHICommandListVK();

        bool Create();

        virtual void* GetNativeHandle() const override { return mCmdBuffer; }

        virtual void ResetAllocator() override;
        virtual void Begin() override;
        virtual void End() override;
        virtual void Wait(RHIFence* fence, uint64_t value) override;
        virtual void Signal(RHIFence* fence, uint64_t value) override;
        virtual void Present(RHISwapchain* swapchain) override;
        virtual void Submit() override;
        virtual void ResetState() override;

        virtual void BeginProfiling() override;
        virtual void EndProfiling() override;
        virtual void BeginEvent(const eastl::string& eventName) override;
        virtual void EndEvent() override;

        virtual void CopyBufferToTexture(RHIBuffer* srcBuffer, RHITexture* dstTexture, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset) override;
        virtual void CopyTextureToBuffer(RHITexture* srcTexture, RHIBuffer* dstBuffer, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset) override;
        virtual void CopyBuffer(RHIBuffer* src, RHIBuffer* dst, uint32_t srcOffset, uint32_t dstOffset, uint32_t size) override;
        virtual void CopyTexture(RHITexture* src, RHITexture* dst, uint32_t srcMipLevel, uint32_t dstMipLevel, uint32_t srcArraySlice, uint32_t dstArraySlice) override;
        virtual void ClearUAV(RHIResource* resource, RHIDescriptor* uav, const float* clearValue) override;
        virtual void ClearUAV(RHIResource* resource, RHIDescriptor* uav, const uint32_t* clearValue) override;
        virtual void WriteBuffer(RHIBuffer* buffer, uint32_t offset, uint32_t data) override;

        virtual void TextureBarrier(RHITexture* texture, uint32_t subResouce, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) override;
        virtual void BufferBarrier(RHIBuffer* buffer, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) override;
        virtual void GlobalBarrier(ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) override;
        virtual void FlushBarriers() override;

        virtual void BeginRenderPass(const RHIRenderPassDesc& desc) override;
        virtual void EndRenderPass() override;
        virtual void SetPipelineState(RHIPipelineState* pipelineState) override;
        virtual void SetStencilReference(uint8_t stencil) override;
        virtual void SetBlendFactor(const float* blendFactor) override;
        virtual void SetIndexBuffer(RHIBuffer* buffer, uint32_t offset, ERHIFormat format) override;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        virtual void SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        virtual void SetGraphicsConstants(uint32_t slot, const void* data, size_t dataSize) override;
        virtual void SetComputeConstants(uint32_t slot, const void* data, size_t dataSize) override;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1) override;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t indexOffset = 0) override;
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
        virtual void DispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;

        virtual void DrawIndirect(RHIBuffer* buffer, uint32_t offset) override;
        virtual void DrawIndexedIndirect(RHIBuffer* buffer, uint32_t offset) override;
        virtual void DispatchIndirect(RHIBuffer* buffer, uint32_t offset) override;

    private:
        void UpdateGraphicsDescriptorBuffer();
        void UpdateComputeDescriptorBuffer();
    
    private:
        vk::Queue mQueue;
        vk::CommandPool mCmdPool;
        vk::CommandBuffer mCmdBuffer;

        eastl::vector<vk::CommandBuffer> mFreeCmdBuffers;
        eastl::vector<vk::CommandBuffer> mPendingCmdBuffers;

        eastl::vector<vk::MemoryBarrier2> mMemoryBarriers;
        eastl::vector<vk::BufferMemoryBarrier2> mBufferMemoryBarriers;
        eastl::vector<vk::ImageMemoryBarrier2> mImageMemoryBarriers;

        eastl::vector<eastl::pair<RHIFence*, uint64_t>> mPendingWaits;
        eastl::vector<eastl::pair<RHIFence*, uint64_t>> mPendingSignals;
        eastl::vector<RHISwapchain*> mPendingSwapchain;

        struct ConstantData
        {
            uint32_t cb0[RHI_MAX_ROOT_CONSTANTS] = {};
            vk::DescriptorAddressInfoEXT cbv1 = {};
            vk::DescriptorAddressInfoEXT cbv2 = {};
        };

        ConstantData mGraphicsConstants;
        ConstantData mComputeConstants;
    };
}