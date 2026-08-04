#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHICommandList.hpp"

namespace RHI
{
    class FVulkanDevice;

    class FVulkanCommandList : public FRHICommandList
    {
    public:
        FVulkanCommandList(FVulkanDevice* device, ERHICommandQueueType queueType, const eastl::string& name);
        ~FVulkanCommandList();

        bool Create();

        virtual void* GetNativeHandle() const override { return m_CmdBuffer; }

        virtual void ResetAllocator() override;
        virtual void Begin() override;
        virtual void End() override;
        virtual void Wait(FRHIFence* fence, uint64_t value) override;
        virtual void Signal(FRHIFence* fence, uint64_t value) override;
        virtual void Present(FRHISwapchain* swapchain) override;
        virtual void Submit() override;
        virtual void ResetState() override;

        virtual void BeginProfiling() override;
        virtual void EndProfiling() override;
        virtual void BeginEvent(const eastl::string& eventName) override;
        virtual void EndEvent() override;

        virtual void CopyBufferToTexture(FRHIBuffer* srcBuffer, FRHITexture* dstTexture, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset) override;
        virtual void CopyTextureToBuffer(FRHITexture* srcTexture, FRHIBuffer* dstBuffer, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset) override;
        virtual void CopyBuffer(FRHIBuffer* src, FRHIBuffer* dst, uint32_t srcOffset, uint32_t dstOffset, uint32_t size) override;
        virtual void CopyTexture(FRHITexture* src, FRHITexture* dst, uint32_t srcMipLevel, uint32_t dstMipLevel, uint32_t srcArraySlice, uint32_t dstArraySlice) override;
        virtual void ClearUAV(FRHIResource* resource, FRHIDescriptor* uav, const float* clearValue) override;
        virtual void ClearUAV(FRHIResource* resource, FRHIDescriptor* uav, const uint32_t* clearValue) override;
        virtual void WriteBuffer(FRHIBuffer* buffer, uint32_t offset, uint32_t data) override;

        virtual void TextureBarrier(FRHITexture* texture, uint32_t subResouce, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) override;
        virtual void BufferBarrier(FRHIBuffer* buffer, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) override;
        virtual void GlobalBarrier(ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter) override;
        virtual void FlushBarriers() override;

        virtual void BeginRenderPass(const FRHIRenderPassDesc& desc) override;
        virtual void EndRenderPass() override;
        virtual void SetPipelineState(FRHIPipelineState* pipelineState) override;
        virtual void SetStencilReference(uint8_t stencil) override;
        virtual void SetBlendFactor(const float* blendFactor) override;
        virtual void SetIndexBuffer(FRHIBuffer* buffer, uint32_t offset, ERHIFormat format) override;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        virtual void SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        virtual void SetGraphicsConstants(uint32_t slot, const void* data, size_t dataSize) override;
        virtual void SetComputeConstants(uint32_t slot, const void* data, size_t dataSize) override;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1) override;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t indexOffset = 0) override;
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
        virtual void DispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;

        virtual void DrawIndirect(FRHIBuffer* buffer, uint32_t offset) override;
        virtual void DrawIndexedIndirect(FRHIBuffer* buffer, uint32_t offset) override;
        virtual void DispatchIndirect(FRHIBuffer* buffer, uint32_t offset) override;
        virtual void DispatchMeshIndirect(FRHIBuffer* buffer, uint32_t offset) override;

        virtual void MultiDrawIndirect(uint32_t maxCount, FRHIBuffer* argsBuffer, uint32_t argsBufferOffset, FRHIBuffer* countBuffer, uint32_t countBufferOffset) override;
        virtual void MultiDrawIndexedIndirect(uint32_t maxCount, FRHIBuffer* argsBuffer, uint32_t argsBufferOffset, FRHIBuffer* countBuffer, uint32_t countBufferOffset) override;
        virtual void MultiDispatchIndirect(uint32_t maxCount, FRHIBuffer* argsBuffer, uint32_t argsBufferOffset, FRHIBuffer* countBuffer, uint32_t countBufferOffset) override;
        virtual void MultiDispatchMeshIndirect(uint32_t maxCount, FRHIBuffer* argsBuffer, uint32_t argsBufferOffset, FRHIBuffer* countBuffer, uint32_t countBufferOffset) override;

    private:
        void UpdateGraphicsDescriptorBuffer();
        void UpdateComputeDescriptorBuffer();
    
    private:
        vk::Queue m_Queue;
        vk::CommandPool m_CmdPool;
        vk::CommandBuffer m_CmdBuffer;
        vk::detail::DispatchLoaderDynamic m_DynamicLoader;

        eastl::vector<vk::CommandBuffer> m_FreeCmdBuffers;
        eastl::vector<vk::CommandBuffer> m_PendingCmdBuffers;

        eastl::vector<vk::MemoryBarrier2> m_MemoryBarriers;
        eastl::vector<vk::BufferMemoryBarrier2> m_BufferMemoryBarriers;
        eastl::vector<vk::ImageMemoryBarrier2> m_ImageMemoryBarriers;

        eastl::vector<eastl::pair<FRHIFence*, uint64_t>> m_PendingWaits;
        eastl::vector<eastl::pair<FRHIFence*, uint64_t>> m_PendingSignals;
        eastl::vector<FRHISwapchain*> m_PendingSwapchain;

        struct FConstantData
        {
            uint32_t cb0[RHI_MAX_ROOT_CONSTANTS] = {};
            vk::DescriptorAddressInfoEXT cbv1 = {};
            vk::DescriptorAddressInfoEXT cbv2 = {};
            bool dirty = false;                     // 标志位，避免ConstantsBuffer的重复绑定
        };

        FConstantData m_GraphicsConstants;
        FConstantData m_ComputeConstants;
    };
}