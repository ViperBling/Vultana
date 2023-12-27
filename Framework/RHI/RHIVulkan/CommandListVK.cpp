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
    static std::tuple<vk::ImageLayout, vk::AccessFlags, vk::PipelineStageFlags> GetBarrierInfo(RHITextureState state)
    {
        switch (state)
        {
        case RHITextureState::Present:
            return { vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eMemoryRead, vk::PipelineStageFlagBits::eBottomOfPipe };
            break;
        case RHITextureState::RenderTarget:
            return { vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput };
            break;
        case RHITextureState::CopyDst:
            return { vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer };
            break;
        case RHITextureState::CopySrc:
            return { vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits::eTransferRead, vk::PipelineStageFlagBits::eTransfer };
            break;
        case RHITextureState::ShaderReadOnly:
            return { vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader };
            break;
        case RHITextureState::DepthStencilReadOnly:
            return { vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::AccessFlagBits::eDepthStencilAttachmentRead, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests };
            break;
        case RHITextureState::DepthStencilWrite:
            return { vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests };
            break;
        default:
            break;
        }
        return { vk::ImageLayout::eUndefined, vk::AccessFlags(), vk::PipelineStageFlagBits::eTopOfPipe };
    }

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
        auto* src = dynamic_cast<BufferVK*>(srcBuffer);
        auto* dst = dynamic_cast<BufferVK*>(dstBuffer);

        vk::BufferCopy copyRegion {};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = size;
        mCommandBuffer.GetVkCommandBuffer().copyBuffer(src->GetVkBuffer(), dst->GetVkBuffer(), 1, &copyRegion);
    }

    void CommandListVK::CopyBufferToTexture(RHIBuffer *Src, RHITexture *Dst, const TextureSubResourceCreateInfo *subResourceInfo, const Vector3 &size)
    {
        auto* buffer = dynamic_cast<BufferVK*>(Src);
        auto* texture = dynamic_cast<TextureVK*>(Dst);

        vk::BufferImageCopy copyRegion {};
        copyRegion.imageExtent = vk::Extent3D { size.x, size.y, size.z };
        copyRegion.imageSubresource = { GetVkAspectMask(subResourceInfo->Type), subResourceInfo->MipLevel, subResourceInfo->BaseArrayLayer, subResourceInfo->LayerCount };

        mCommandBuffer.GetVkCommandBuffer().copyBufferToImage(buffer->GetVkBuffer(), texture->GetVkImage(), vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
    }

    void CommandListVK::CopyTextureToBuffer(RHITexture *Src, RHIBuffer *Dst, const TextureSubResourceCreateInfo *subResourceInfo, const Vector3 &size)
    {
        auto* buffer = dynamic_cast<BufferVK*>(Dst);
        auto* texture = dynamic_cast<TextureVK*>(Src);

        vk::BufferImageCopy copyRegion {};
        copyRegion.imageExtent = vk::Extent3D { size.x, size.y, size.z };
        copyRegion.imageSubresource = { GetVkAspectMask(subResourceInfo->Type), subResourceInfo->MipLevel, subResourceInfo->BaseArrayLayer, subResourceInfo->LayerCount };

        mCommandBuffer.GetVkCommandBuffer().copyImageToBuffer(texture->GetVkImage(), vk::ImageLayout::eTransferSrcOptimal, buffer->GetVkBuffer(), 1, &copyRegion);
    }

    void CommandListVK::CopyTextureToTexture(RHITexture *Src, RHITexture *Dst, const TextureSubResourceCreateInfo *srcSubResourceInfo, const TextureSubResourceCreateInfo *dstSubResourceInfo, const Vector3 &size)
    {
        auto* srcTexture = dynamic_cast<TextureVK*>(Src);
        auto* dstTexture = dynamic_cast<TextureVK*>(Dst);

        vk::ImageCopy copyRegion {};
        copyRegion.extent = vk::Extent3D { size.x, size.y, size.z };
        copyRegion.srcSubresource = { GetVkAspectMask(srcSubResourceInfo->Type), srcSubResourceInfo->MipLevel, srcSubResourceInfo->BaseArrayLayer, srcSubResourceInfo->LayerCount };
        copyRegion.dstSubresource = { GetVkAspectMask(dstSubResourceInfo->Type), dstSubResourceInfo->MipLevel, dstSubResourceInfo->BaseArrayLayer, dstSubResourceInfo->LayerCount };

        mCommandBuffer.GetVkCommandBuffer().copyImage(srcTexture->GetVkImage(), vk::ImageLayout::eTransferSrcOptimal, dstTexture->GetVkImage(), vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
    }

    void CommandListVK::ResourceBarrier(const RHIBarrier &barrier)
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