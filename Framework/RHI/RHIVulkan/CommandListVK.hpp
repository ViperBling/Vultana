#pragma once

#include "RHI/RHICommandList.hpp"

#include <vulkan/vulkan.hpp>

namespace RHI
{
    class DeviceVK;
    class GPUVK;
    class CommandBufferVK;
    class GraphicsPipelineVK;

    class CommandListVK : public RHICommandList
    {
    public:
        NOCOPY(CommandListVK);
        CommandListVK(DeviceVK& device, CommandBufferVK& commandBuffer);
        ~CommandListVK();

        virtual void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64_t srcOffset, uint64_t dstOffset, uint64_t size) override;
        virtual void CopyBufferToTexture(RHIBuffer* Src, RHITexture* Dst, const TextureSubResourceCreateInfo* subResourceInfo, const Math::Vector3u& size) override;
        virtual void CopyTextureToBuffer(RHITexture* Src, RHIBuffer* Dst, const TextureSubResourceCreateInfo* subResourceInfo, const Math::Vector3u& size) override;
        virtual void CopyTextureToTexture(RHITexture* Src, RHITexture* Dst, const TextureSubResourceCreateInfo* srcSubResourceInfo, const TextureSubResourceCreateInfo* dstSubResourceInfo, const Math::Vector3u& size) override;
        virtual void ResourceBarrier(const RHIBarrier& barrier) override;

        virtual RHIComputePassCommandList* BeginComputePass() override;
        virtual RHIGraphicsPassCommandList* BeginGraphicsPass(const GraphicsPassBeginInfo* info) override;
        virtual void SwapchainSync(RHISwapchain* swapchain) override;
        virtual void End() override;
        void Destroy() override;
    
    private:
        DeviceVK& mDevice;
        CommandBufferVK& mCommandBuffer;
    };

    class ComputePassCommandListVK : public RHIComputePassCommandList
    {
    public:
        NOCOPY(ComputePassCommandListVK);
        explicit ComputePassCommandListVK(DeviceVK& device, CommandBufferVK& commandBuffer);
        ~ComputePassCommandListVK();
        
        virtual void SetPipeline(RHIComputePipeline* pipeline) override {}
        virtual void SetBindGroup(uint32_t index, RHIBindGroup* bindGroup) override {}
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override {}
        virtual void EndPass() override {}
        virtual void Destroy() override {}
    
    private:
        DeviceVK& mDevice;
        CommandBufferVK& mCommandBuffer;
    };

    class GraphicsPassCommandListVK : public RHIGraphicsPassCommandList
    {
    public:
        NOCOPY(GraphicsPassCommandListVK);
        explicit GraphicsPassCommandListVK(DeviceVK& device, CommandBufferVK& commandBuffer, const GraphicsPassBeginInfo* beginInfo);
        ~GraphicsPassCommandListVK();

        virtual void SetPipeline(RHIGraphicsPipeline* pipeline) override;
        virtual void SetBindGroup(uint32_t index, RHIBindGroup* bindGroup) override;
        virtual void SetIndexBuffer(RHIBufferView* bufferView) override;
        virtual void SetVertexBuffers(uint32_t slot, RHIBufferView* bufferView) override;
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t firstInstance) override;
        virtual void SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth) override;
        virtual void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) override;
        virtual void SetPrimitiveTopology(RHIPrimitiveTopologyType topology) override;
        virtual void SetBlendConstants(const float blendConstants[4]) override;
        virtual void SetStencilReference(uint32_t reference) override;

        virtual void EndPass() override;
        virtual void Destroy() override;

    private:
        DeviceVK& mDevice;
        CommandBufferVK& mCommandBuffer;
        vk::CommandBuffer mCmdHandle;
        GraphicsPipelineVK* mGraphicsPipeline;
    };
} // namespace Vultana
