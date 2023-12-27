#include "CommandListVK.hpp"

#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"
#include "GPUVK.hpp"
#include "CommandBufferVK.hpp"
#include "BufferVK.hpp"
#include "BufferViewVK.hpp"
#include "TextureVK.hpp"
#include "TextureViewVK.hpp"
#include "InstanceVK.hpp"
#include "SwapchainVK.hpp"
#include "BindGroupVK.hpp"
#include "PipelineLayoutVK.hpp"

#include "RHI/RHISynchronous.hpp"

namespace Vultana
{
    CommandListVK::CommandListVK(DeviceVK &device, CommandBufferVK &commandBuffer)
        : mDevice(device)
        , mCommandBuffer(commandBuffer)
    {
    }

    CommandListVK::~CommandListVK()
    {
    }

    void CommandListVK::CopyBufferToBuffer(RHIBuffer *srcBuffer, RHIBuffer *dstBuffer, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
    }

    void CommandListVK::CopyBufferToTexture(RHIBuffer *Src, RHITexture *Dst, const TextureSubResourceCreateInfo *subResourceInfo, const Vector3 &size)
    {

    }

    void CommandListVK::CopyTextureToBuffer(RHITexture *Src, RHIBuffer *Dst, const TextureSubResourceCreateInfo *subResourceInfo, const Vector3 &size)
    {
    }

    void CommandListVK::CopyTextureToTexture(RHITexture *Src, RHITexture *Dst, const TextureSubResourceCreateInfo *srcSubResourceInfo, const TextureSubResourceCreateInfo *dstSubResourceInfo, const Vector3 &size)
    {
    }

    RHIComputePassCommandList *CommandListVK::BeginComputePass()
    {
        return nullptr;
    }

    RHIGraphicsPassCommandList *CommandListVK::BeginGraphicsPass(const GraphicsPassBeginInfo &info)
    {
        return nullptr;
    }

    void CommandListVK::SwapchainSync(RHISwapchain *swapchain)
    {

        
    }

    void CommandListVK::End()
    {
    }

    void CommandListVK::Destroy()
    {
    }

    // ================================= ComputePassCommandListVK ================================= //
    ComputePassCommandListVK::ComputePassCommandListVK(DeviceVK & device, CommandBufferVK & commandBuffer)
        : mDevice(device)
        , mCommandBuffer(commandBuffer)
    {
    }
    ComputePassCommandListVK::~ComputePassCommandListVK()
    {
    }

    // ================================= GraphicsPassCommandListVK ================================= //
    GraphicsPassCommandListVK::GraphicsPassCommandListVK(DeviceVK &device, CommandBufferVK &commandBuffer, const GraphicsPassBeginInfo *beginInfo)
        : mDevice(device)
        , mCommandBuffer(commandBuffer)
    {
    }

    GraphicsPassCommandListVK::~GraphicsPassCommandListVK()
    {
    }

    void GraphicsPassCommandListVK::SetPipeline(RHIGraphicsPipeline *pipeline)
    {
    }

    void GraphicsPassCommandListVK::SetBindGroup(uint32_t index, RHIBindGroup *bindGroup)
    {
    }

    void GraphicsPassCommandListVK::SetIndexBuffer(RHIBufferView *bufferView)
    {
    }

    void GraphicsPassCommandListVK::SetVertexBuffers(uint32_t slot, RHIBufferView *bufferView)
    {
    }

    void GraphicsPassCommandListVK::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
    }

    void GraphicsPassCommandListVK::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t firstInstance)
    {
    }

    void GraphicsPassCommandListVK::SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
    {
    }

    void GraphicsPassCommandListVK::SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
    {
    }

    void GraphicsPassCommandListVK::SetPrimitiveTopology(RHIPrimitiveTopology topology)
    {

        
    }

    void GraphicsPassCommandListVK::SetBlendConstants(const float blendConstants[4])
    {
    }

    void GraphicsPassCommandListVK::SetStencilReference(uint32_t reference)
    {
    }

    void GraphicsPassCommandListVK::EndPass()
    {
    }

    void GraphicsPassCommandListVK::Destroy()
    {
    }
}