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
#include "PipelineVK.hpp"
#include "PipelineLayoutVK.hpp"

#include "RHI/RHISynchronous.hpp"

namespace RHI
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
        Destroy();
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

    void CommandListVK::CopyBufferToTexture(RHIBuffer *Src, RHITexture *Dst, const TextureSubResourceCreateInfo *subResourceInfo, const Math::Vector3u &size)
    {
        auto* buffer = dynamic_cast<BufferVK*>(Src);
        auto* texture = dynamic_cast<TextureVK*>(Dst);

        vk::BufferImageCopy copyRegion {};
        copyRegion.imageExtent = vk::Extent3D { size.x, size.y, size.z };
        copyRegion.imageSubresource = { GetVkAspectMask(subResourceInfo->Type), subResourceInfo->MipLevel, subResourceInfo->BaseArrayLayer, subResourceInfo->LayerCount };

        mCommandBuffer.GetVkCommandBuffer().copyBufferToImage(buffer->GetVkBuffer(), texture->GetVkImage(), vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
    }

    void CommandListVK::CopyTextureToBuffer(RHITexture *Src, RHIBuffer *Dst, const TextureSubResourceCreateInfo *subResourceInfo, const Math::Vector3u &size)
    {
        auto* buffer = dynamic_cast<BufferVK*>(Dst);
        auto* texture = dynamic_cast<TextureVK*>(Src);

        vk::BufferImageCopy copyRegion {};
        copyRegion.imageExtent = vk::Extent3D { size.x, size.y, size.z };
        copyRegion.imageSubresource = { GetVkAspectMask(subResourceInfo->Type), subResourceInfo->MipLevel, subResourceInfo->BaseArrayLayer, subResourceInfo->LayerCount };

        mCommandBuffer.GetVkCommandBuffer().copyImageToBuffer(texture->GetVkImage(), vk::ImageLayout::eTransferSrcOptimal, buffer->GetVkBuffer(), 1, &copyRegion);
    }

    void CommandListVK::CopyTextureToTexture(RHITexture *Src, RHITexture *Dst, const TextureSubResourceCreateInfo *srcSubResourceInfo, const TextureSubResourceCreateInfo *dstSubResourceInfo, const Math::Vector3u &size)
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
        if (barrier.Type == RHIResourceType::Texture)
        {
            const auto& textureBarrierInfo = barrier.Texture;
            auto oldLayout = GetBarrierInfo(textureBarrierInfo.Before == RHITextureState::Present ? RHITextureState::Undefined : textureBarrierInfo.Before);
            auto newLayout = GetBarrierInfo(textureBarrierInfo.After);

            auto* textureVK = static_cast<TextureVK*>(textureBarrierInfo.Texture);
            vk::ImageMemoryBarrier imageBarrier {};
            imageBarrier.oldLayout = std::get<0>(oldLayout);
            imageBarrier.newLayout = std::get<0>(newLayout);
            imageBarrier.srcAccessMask = std::get<1>(oldLayout);
            imageBarrier.dstAccessMask = std::get<1>(newLayout);
            imageBarrier.image = textureVK->GetVkImage();
            imageBarrier.subresourceRange = textureVK->GetFullRange();
            mCommandBuffer.GetVkCommandBuffer().pipelineBarrier(std::get<2>(oldLayout), std::get<2>(newLayout), vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &imageBarrier);
        }
    }

    RHIComputePassCommandList *CommandListVK::BeginComputePass()
    {
        return nullptr;
    }

    RHIGraphicsPassCommandList *CommandListVK::BeginGraphicsPass(const GraphicsPassBeginInfo* info)
    {
        return new GraphicsPassCommandListVK(mDevice, mCommandBuffer, info);
    }

    void CommandListVK::SwapchainSync(RHISwapchain *swapchain)
    {
        auto swapchainVK = static_cast<SwapchainVK*>(swapchain);
        auto signal = swapchainVK->GetImageSemaphore();
        mCommandBuffer.AddWaitSemaphore(signal, vk::PipelineStageFlagBits::eColorAttachmentOutput);
        swapchainVK->AddWaitSemaphore(mCommandBuffer.GetSignalSemaphores()[0]);
    }

    void CommandListVK::End()
    {
        mCommandBuffer.GetVkCommandBuffer().end();
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
        std::vector<vk::RenderingAttachmentInfo> colorAttachInfos(beginInfo->ColorAttachmentCount);
        for (size_t i = 0; i < beginInfo->ColorAttachmentCount; i++)
        {
            auto* colorTexViewVK = dynamic_cast<TextureViewVK*>(beginInfo->ColorAttachments[i].TextureView);
            colorAttachInfos[i].setImageView(colorTexViewVK->GetVkImageView());
            colorAttachInfos[i].setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
            colorAttachInfos[i].setLoadOp(VKEnumCast<RHILoadOp, vk::AttachmentLoadOp>(beginInfo->ColorAttachments[i].LoadOp));
            colorAttachInfos[i].setStoreOp(VKEnumCast<RHIStoreOp, vk::AttachmentStoreOp>(beginInfo->ColorAttachments[i].StoreOp));
            colorAttachInfos[i].clearValue.color = {
                beginInfo->ColorAttachments[i].ColorClearValue.r,
                beginInfo->ColorAttachments[i].ColorClearValue.g,
                beginInfo->ColorAttachments[i].ColorClearValue.b,
                beginInfo->ColorAttachments[i].ColorClearValue.a
            };
        }
        auto* textureView = dynamic_cast<TextureViewVK*>(beginInfo->ColorAttachments[0].TextureView);
        vk::RenderingInfoKHR renderInfo {};
        renderInfo.setColorAttachments(colorAttachInfos);
        renderInfo.setLayerCount(textureView->GetArrayLayerCount());
        renderInfo.setRenderArea(vk::Rect2D { { 0, 0 }, { static_cast<uint32_t>(textureView->GetTexture().GetExtent().x), static_cast<uint32_t>(textureView->GetTexture().GetExtent().y) } });
        renderInfo.viewMask = 0;

        if (beginInfo->DepthStencilAttachment != nullptr)
        {
            auto* depthStencilTexView = dynamic_cast<TextureViewVK*>(beginInfo->DepthStencilAttachment->TextureView);
            vk::RenderingAttachmentInfo depthStencilAttachInfo {};
            depthStencilAttachInfo.setImageView(depthStencilTexView->GetVkImageView());
            depthStencilAttachInfo.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
            depthStencilAttachInfo.setLoadOp(VKEnumCast<RHILoadOp, vk::AttachmentLoadOp>(beginInfo->DepthStencilAttachment->DepthLoadOp));
            depthStencilAttachInfo.setStoreOp(VKEnumCast<RHIStoreOp, vk::AttachmentStoreOp>(beginInfo->DepthStencilAttachment->DepthStoreOp));
            depthStencilAttachInfo.clearValue.setDepthStencil({ beginInfo->DepthStencilAttachment->DepthClearValue, beginInfo->DepthStencilAttachment->StencilClearValue });

            renderInfo.setPDepthAttachment(&depthStencilAttachInfo);

            if (!beginInfo->DepthStencilAttachment->DepthReadOnly)
            {
                vk::RenderingAttachmentInfo stencilAttachInfo {};
                stencilAttachInfo.setImageView(depthStencilTexView->GetVkImageView());
                stencilAttachInfo.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
                stencilAttachInfo.setLoadOp(VKEnumCast<RHILoadOp, vk::AttachmentLoadOp>(beginInfo->DepthStencilAttachment->StencilLoadOp));
                stencilAttachInfo.setStoreOp(VKEnumCast<RHIStoreOp, vk::AttachmentStoreOp>(beginInfo->DepthStencilAttachment->StencilStoreOp));
                stencilAttachInfo.clearValue.setDepthStencil({ beginInfo->DepthStencilAttachment->DepthClearValue, beginInfo->DepthStencilAttachment->StencilClearValue });

                renderInfo.setPStencilAttachment(&stencilAttachInfo);
            }
        }
        mCmdHandle = commandBuffer.GetVkCommandBuffer();
        VkRenderingInfoKHR vkRenderInfo = VkRenderingInfoKHR(renderInfo);
        mDevice.GetGPU().GetInstance().GetVkDynamicLoader().vkCmdBeginRendering(mCmdHandle, &vkRenderInfo);
    }

    GraphicsPassCommandListVK::~GraphicsPassCommandListVK()
    {
    }

    void GraphicsPassCommandListVK::SetPipeline(RHIGraphicsPipeline *pipeline)
    {
        mGraphicsPipeline = dynamic_cast<GraphicsPipelineVK*>(pipeline);
        assert(mGraphicsPipeline != nullptr);
        mCmdHandle.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline->GetVkPipeline());
    }

    void GraphicsPassCommandListVK::SetBindGroup(uint32_t index, RHIBindGroup *bindGroup)
    {
        auto* bg = dynamic_cast<BindGroupVK*>(bindGroup);
        assert(bg != nullptr);
        vk::DescriptorSet descSet = bg->GetVkDescriptorSet();
        vk::PipelineLayout layout = mGraphicsPipeline->GetPipelineLayout()->GetVkPipelineLayout();
        mCmdHandle.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, index, 1, &descSet, 0, nullptr);
    }

    void GraphicsPassCommandListVK::SetIndexBuffer(RHIBufferView *bufferView)
    {
        auto* bv = dynamic_cast<BufferViewVK*>(bufferView);
        assert(bv != nullptr);
        vk::Buffer buffer = bv->GetBuffer().GetVkBuffer();
        auto formatVK = VKEnumCast<RHIIndexFormat, vk::IndexType>(bv->GetIndexFormat());
        mCmdHandle.bindIndexBuffer(buffer, 0, formatVK);
    }

    void GraphicsPassCommandListVK::SetVertexBuffers(uint32_t slot, RHIBufferView *bufferView)
    {
        auto* bv = dynamic_cast<BufferViewVK*>(bufferView);
        assert(bv != nullptr);
        vk::Buffer buffer = bv->GetBuffer().GetVkBuffer();
        vk::DeviceSize offset[] = { bv->GetOffset() };
        mCmdHandle.bindVertexBuffers(slot, 1, &buffer, offset);
    }

    void GraphicsPassCommandListVK::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        mCmdHandle.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void GraphicsPassCommandListVK::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t firstInstance)
    {
        mCmdHandle.drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }

    void GraphicsPassCommandListVK::SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
    {
        vk::Viewport viewport {};
        viewport.x = topLeftX;
        viewport.y = topLeftY;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;
        mCmdHandle.setViewport(0, 1, &viewport);
    }

    void GraphicsPassCommandListVK::SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
    {
        vk::Rect2D scissor {};
        scissor.setOffset({ static_cast<int32_t>(left), static_cast<int32_t>(top) });
        scissor.setExtent({ right - left, bottom - top });
        mCmdHandle.setScissor(0, 1, &scissor);
    }

    void GraphicsPassCommandListVK::SetPrimitiveTopology(RHIPrimitiveTopologyType topology)
    {
        mCmdHandle.setPrimitiveTopology(VKEnumCast<RHIPrimitiveTopologyType, vk::PrimitiveTopology>(topology));
    }

    void GraphicsPassCommandListVK::SetBlendConstants(const float blendConstants[4])
    {
        mCmdHandle.setBlendConstants(blendConstants);
    }

    void GraphicsPassCommandListVK::SetStencilReference(uint32_t reference)
    {
        mCmdHandle.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, reference);
    }

    void GraphicsPassCommandListVK::EndPass()
    {
        mDevice.GetGPU().GetInstance().GetVkDynamicLoader().vkCmdEndRendering(mCmdHandle);
    }

    void GraphicsPassCommandListVK::Destroy()
    {
    }
}