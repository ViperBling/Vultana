#include "RHICommandListVK.hpp"
#include "RHIDescriptorVK.hpp"
#include "RHIDescriptorAllocatorVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIHeapVK.hpp"
#include "RHIFenceVK.hpp"
#include "RHISwapchainVK.hpp"
#include "RHITextureVK.hpp"

#include "RHI/RHI.hpp"

#include "Renderer/ClearUAV.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHICommandListVK::RHICommandListVK(RHIDeviceVK *device, ERHICommandQueueType queueType, const std::string &name)
    {
        mpDevice = device;
        mCmdQueueType = queueType;
        mName = name;
    }

    RHICommandListVK::~RHICommandListVK()
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mCmdPool);
    }

    bool RHICommandListVK::Create()
    {
        auto device = (RHIDeviceVK*)mpDevice;

        vk::CommandPoolCreateInfo cmdPoolCI {};

        switch (mCmdQueueType)
        {
        case ERHICommandQueueType::Graphics:
            cmdPoolCI.setQueueFamilyIndex(device->GetGraphicsQueueIndex());
            mQueue = device->GetGraphicsQueue();
            break;
        case ERHICommandQueueType::Compute:
            cmdPoolCI.setQueueFamilyIndex(device->GetComputeQueueIndex());
            mQueue = device->GetComputeQueue();
            break;
        case ERHICommandQueueType::Copy:
            cmdPoolCI.setQueueFamilyIndex(device->GetCopyQueueIndex());
            mQueue = device->GetCopyQueue();
            break;
        default:
            break;
        }

        vk::Device deviceHandle = device->GetDevice();
        auto dynamicLoader = device->GetDynamicLoader();
        vk::Result res = deviceHandle.createCommandPool(&cmdPoolCI, nullptr, &mCmdPool);
        if (res != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHICommandListVK] Failed to create command pool");
            return false;
        }
        SetDebugName(deviceHandle, vk::ObjectType::eCommandPool, (uint64_t)(VkCommandPool)mCmdPool, mName.c_str(), dynamicLoader);

        return true;
    }

    void RHICommandListVK::ResetAllocator()
    {
        vk::Device deviceHandle = ((RHIDeviceVK*)mpDevice)->GetDevice();
        deviceHandle.resetCommandPool(mCmdPool);

        for (size_t i = 0; i < mPendingCmdBuffers.size(); i++)
        {
            mFreeCmdBuffers.push_back(mPendingCmdBuffers[i]);
        }
        mPendingCmdBuffers.clear();
    }

    void RHICommandListVK::Begin()
    {
        vk::Result res;

        if (!mFreeCmdBuffers.empty())
        {
            mCmdBuffer = mFreeCmdBuffers.back();
            mFreeCmdBuffers.pop_back();
        }
        else
        {
            vk::CommandBufferAllocateInfo cmdBufferAI {};
            cmdBufferAI.setCommandPool(mCmdPool);
            cmdBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
            cmdBufferAI.setCommandBufferCount(1);

            vk::Device deviceHandle = ((RHIDeviceVK*)mpDevice)->GetDevice();
            res = deviceHandle.allocateCommandBuffers(&cmdBufferAI, &mCmdBuffer);
            if (res != vk::Result::eSuccess)
            {
                VTNA_LOG_ERROR("[RHICommandListVK] Failed to allocate command buffer");
                return;
            }
        }
        vk::CommandBufferBeginInfo cmdBufferBI {};
        cmdBufferBI.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        mCmdBuffer.begin(cmdBufferBI);

        ResetState();
    }

    void RHICommandListVK::End()
    {
        FlushBarriers();

        mCmdBuffer.end();
        mPendingCmdBuffers.push_back(mCmdBuffer);
    }

    void RHICommandListVK::Wait(RHIFence *fence, uint64_t value)
    {
        mPendingWaits.emplace_back(fence, value);
    }

    void RHICommandListVK::Signal(RHIFence *fence, uint64_t value)
    {
        mPendingSignals.emplace_back(fence, value);
    }

    void RHICommandListVK::Present(RHISwapchain *swapchain)
    {
        mPendingSwapchain.push_back(swapchain);
    }

    void RHICommandListVK::Submit()
    {
        ((RHIDeviceVK*)mpDevice)->FlushLayoutTransition(mCmdQueueType);

        std::vector<vk::Semaphore> waitSemaphores;
        std::vector<vk::Semaphore> signalSemaphores;
        std::vector<uint64_t> waitValues;
        std::vector<uint64_t> signalValues;
        std::vector<vk::PipelineStageFlags> waitStages;

        for (size_t i = 0; i < mPendingWaits.size(); i++)
        {
            waitSemaphores.push_back((VkSemaphore)mPendingWaits[i].first->GetNativeHandle());
            waitStages.push_back(vk::PipelineStageFlagBits::eTopOfPipe);
            waitValues.push_back(mPendingWaits[i].second);
        }
        mPendingWaits.clear();

        for (size_t i = 0; i < mPendingSignals.size(); i++)
        {
            signalSemaphores.push_back((VkSemaphore)mPendingSignals[i].first->GetNativeHandle());
            signalValues.push_back(mPendingSignals[i].second);
        }
        mPendingSignals.clear();

        for (size_t i = 0; i < mPendingSwapchain.size(); i++)
        {
            auto swapchain = (RHISwapchainVK*)mPendingSwapchain[i];
            waitSemaphores.push_back(swapchain->GetAcquireSemaphore());
            waitStages.push_back(vk::PipelineStageFlagBits::eTopOfPipe);
            signalSemaphores.push_back(swapchain->GetPresentSemaphore());

            waitValues.push_back(0);
            signalValues.push_back(0);
        }

        vk::TimelineSemaphoreSubmitInfo timelineSI {};
        timelineSI.setWaitSemaphoreValues(waitValues);
        timelineSI.setSignalSemaphoreValues(signalValues);

        vk::SubmitInfo submitInfo {};
        submitInfo.setPNext(&timelineSI);
        submitInfo.setWaitSemaphores(waitSemaphores);
        submitInfo.setSignalSemaphores(signalSemaphores);
        submitInfo.setWaitDstStageMask(waitStages);
        submitInfo.setCommandBuffers(mCmdBuffer);

        mQueue.submit(submitInfo);

        for (size_t i = 0; i < mPendingSwapchain.size(); i++)
        {
            auto swapchain = (RHISwapchainVK*)mPendingSwapchain[i];
            swapchain->Present(mQueue);
        }
        mPendingSwapchain.clear();
    }
    
    void RHICommandListVK::ResetState()
    {
        if (mCmdQueueType == ERHICommandQueueType::Graphics || mCmdQueueType == ERHICommandQueueType::Compute)
        {
            auto device = (RHIDeviceVK*)mpDevice;
            auto dynamicLoader = device->GetDynamicLoader();

            vk::DescriptorBufferBindingInfoEXT descBufferBI[3] {};
            descBufferBI[0].setAddress(device->GetConstantBufferAllocator()->GetGPUAddress());
            descBufferBI[0].setUsage(vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
            descBufferBI[1].setAddress(device->GetResourceDescriptorAllocator()->GetGPUAddress());
            descBufferBI[1].setUsage(vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
            descBufferBI[2].setAddress(device->GetSamplerDescriptorAllocator()->GetGPUAddress());
            descBufferBI[2].setUsage(vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT);

            mCmdBuffer.bindDescriptorBuffersEXT(3, descBufferBI, dynamicLoader);
        }
    }

    void RHICommandListVK::BeginProfiling()
    {
    }

    void RHICommandListVK::EndProfiling()
    {
    }

    void RHICommandListVK::BeginEvent(const std::string &eventName)
    {
        vk::DebugUtilsLabelEXT label {};
        label.pLabelName = eventName.c_str();

        auto dynamicLoader = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();
        mCmdBuffer.beginDebugUtilsLabelEXT(label, dynamicLoader);
    }

    void RHICommandListVK::EndEvent()
    {
        auto dynamicLoader = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();
        mCmdBuffer.endDebugUtilsLabelEXT(dynamicLoader);
    }

    void RHICommandListVK::CopyBufferToTexture(RHIBuffer *srcBuffer, RHITexture *dstTexture, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset)
    {
        FlushBarriers();

        const RHITextureDesc& desc = dstTexture->GetDesc();

        vk::BufferImageCopy2 copy2 {};
        copy2.bufferOffset = offset;
        copy2.imageSubresource.aspectMask = GetAspectFlags(desc.Format);
        copy2.imageSubresource.mipLevel = mipLevel;
        copy2.imageSubresource.baseArrayLayer = arraySlice;
        copy2.imageSubresource.layerCount = 1;
        copy2.imageExtent.width = std::max(desc.Width >> mipLevel, 1u);
        copy2.imageExtent.height = std::max(desc.Height >> mipLevel, 1u);
        copy2.imageExtent.depth = std::max(desc.Depth >> mipLevel, 1u);

        vk::CopyBufferToImageInfo2 copyInfo {};
        copyInfo.srcBuffer = (VkBuffer)srcBuffer->GetNativeHandle();
        copyInfo.dstImage = (VkImage)dstTexture->GetNativeHandle();
        copyInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &copy2;

        mCmdBuffer.copyBufferToImage2(copyInfo);
    }

    void RHICommandListVK::CopyTextureToBuffer(RHITexture *srcTexture, RHIBuffer *dstBuffer, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset)
    {
        FlushBarriers();

        const RHITextureDesc& desc = srcTexture->GetDesc();

        vk::BufferImageCopy2 copy2 {};
        copy2.bufferOffset = offset;
        copy2.imageSubresource.aspectMask = GetAspectFlags(desc.Format);
        copy2.imageSubresource.mipLevel = mipLevel;
        copy2.imageSubresource.baseArrayLayer = arraySlice;
        copy2.imageSubresource.layerCount = 1;
        copy2.imageExtent.width = std::max(desc.Width >> mipLevel, 1u);
        copy2.imageExtent.height = std::max(desc.Height >> mipLevel, 1u);
        copy2.imageExtent.depth = std::max(desc.Depth >> mipLevel, 1u);

        vk::CopyImageToBufferInfo2 copyInfo {};
        copyInfo.srcImage = (VkImage)srcTexture->GetNativeHandle();
        copyInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
        copyInfo.dstBuffer = (VkBuffer)dstBuffer->GetNativeHandle();
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &copy2;

        mCmdBuffer.copyImageToBuffer2(copyInfo);
    }

    void RHICommandListVK::CopyBuffer(RHIBuffer *src, RHIBuffer *dst, uint32_t srcOffset, uint32_t dstOffset, uint32_t size)
    {
        FlushBarriers();

        vk::BufferCopy2 copy2 {};
        copy2.srcOffset = srcOffset;
        copy2.dstOffset = dstOffset;
        copy2.size = size;

        vk::CopyBufferInfo2 copyInfo2 {};
        copyInfo2.srcBuffer = (VkBuffer)src->GetNativeHandle();
        copyInfo2.dstBuffer = (VkBuffer)dst->GetNativeHandle();
        copyInfo2.regionCount = 1;
        copyInfo2.pRegions = &copy2;

        mCmdBuffer.copyBuffer2(copyInfo2);
    }

    void RHICommandListVK::CopyTexture(RHITexture *src, RHITexture *dst, uint32_t srcMipLevel, uint32_t dstMipLevel, uint32_t srcArraySlice, uint32_t dstArraySlice)
    {
        FlushBarriers();

        vk::ImageCopy2 copy2 {};
        copy2.srcSubresource.aspectMask = GetAspectFlags(src->GetDesc().Format);
        copy2.srcSubresource.mipLevel = srcMipLevel;
        copy2.srcSubresource.baseArrayLayer = srcArraySlice;
        copy2.srcSubresource.layerCount = 1;
        copy2.dstSubresource.aspectMask = GetAspectFlags(dst->GetDesc().Format);
        copy2.dstSubresource.mipLevel = dstMipLevel;
        copy2.dstSubresource.baseArrayLayer = dstArraySlice;
        copy2.dstSubresource.layerCount = 1;
        copy2.extent.width = std::max(src->GetDesc().Width >> srcMipLevel, 1u);
        copy2.extent.height = std::max(src->GetDesc().Height >> srcMipLevel, 1u);
        copy2.extent.depth = std::max(src->GetDesc().Depth >> srcMipLevel, 1u);

        vk::CopyImageInfo2 copyInfo2 {};
        copyInfo2.setSrcImage((VkImage)src->GetNativeHandle());
        copyInfo2.setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal);
        copyInfo2.setDstImage((VkImage)dst->GetNativeHandle());
        copyInfo2.setDstImageLayout(vk::ImageLayout::eTransferDstOptimal);
        copyInfo2.regionCount = 1;
        copyInfo2.pRegions = &copy2;

        mCmdBuffer.copyImage2(copyInfo2);
    }

    void RHICommandListVK::ClearUAV(RHIResource *resource, RHIDescriptor *uav, const float *clearValue)
    {
        const RHI::RHIUnorderedAccessViewDesc& uavDesc = static_cast<RHI::RHIUnorderedAccessViewVK*>(uav)->GetDesc();
        Renderer::ClearUAV(this, resource, uav, uavDesc, clearValue);
    }

    void RHICommandListVK::ClearUAV(RHIResource *resource, RHIDescriptor *uav, const uint32_t *clearValue)
    {
        const RHI::RHIUnorderedAccessViewDesc& uavDesc = static_cast<RHI::RHIUnorderedAccessViewVK*>(uav)->GetDesc();
        Renderer::ClearUAV(this, resource, uav, uavDesc, clearValue);
    }

    void RHICommandListVK::WriteBuffer(RHIBuffer *buffer, uint32_t offset, uint32_t data)
    {
        FlushBarriers();

        mCmdBuffer.updateBuffer((VkBuffer)buffer->GetNativeHandle(), offset, sizeof(uint32_t), &data);
    }

    void RHICommandListVK::TextureBarrier(RHITexture *texture, uint32_t subResouce, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter)
    {
        vk::ImageMemoryBarrier2 barrier {};
        barrier.setImage((VkImage)texture->GetNativeHandle());
        barrier.setSrcStageMask(GetStageMask(accessFlagBefore));
        barrier.setDstStageMask(GetStageMask(accessFlagAfter));
        barrier.setSrcAccessMask(GetAccessMask(accessFlagBefore));
        barrier.setDstAccessMask(GetAccessMask(accessFlagAfter));
        barrier.setOldLayout(GetImageLayout(accessFlagBefore));
        if (accessFlagAfter & RHIAccessDiscard)
        {
            barrier.setNewLayout(barrier.oldLayout);
        }
        else
        {
            barrier.setNewLayout(GetImageLayout(accessFlagAfter));
        }
        barrier.subresourceRange.aspectMask = GetAspectFlags(texture->GetDesc().Format);

        if (subResouce == RHI_ALL_SUB_RESOURCE)
        {
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = vk::RemainingMipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = vk::RemainingArrayLayers;
        }
        else
        {
            uint32_t mip, slice;
            DecomposeSubresource(texture->GetDesc(), subResouce, mip, slice);

            barrier.subresourceRange.baseMipLevel = mip;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = slice;
            barrier.subresourceRange.layerCount = 1;
        }

        mImageMemoryBarriers.push_back(barrier);
    }

    void RHICommandListVK::BufferBarrier(RHIBuffer *buffer, ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter)
    {
        vk::BufferMemoryBarrier2 barrier2 {};
        barrier2.setBuffer((VkBuffer)buffer->GetNativeHandle());
        barrier2.setOffset(0);
        barrier2.setSize(buffer->GetDesc().Size);
        barrier2.setSrcStageMask(GetStageMask(accessFlagBefore));
        barrier2.setDstStageMask(GetStageMask(accessFlagAfter));
        barrier2.setSrcAccessMask(GetAccessMask(accessFlagBefore));
        barrier2.setDstAccessMask(GetAccessMask(accessFlagAfter));

        mBufferMemoryBarriers.push_back(barrier2);
    }

    void RHICommandListVK::GlobalBarrier(ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter)
    {
        vk::MemoryBarrier2 barrier2 {};
        barrier2.setSrcStageMask(GetStageMask(accessFlagBefore));
        barrier2.setDstStageMask(GetStageMask(accessFlagAfter));
        barrier2.setSrcAccessMask(GetAccessMask(accessFlagBefore));
        barrier2.setDstAccessMask(GetAccessMask(accessFlagAfter));

        mMemoryBarriers.push_back(barrier2);
    }

    void RHICommandListVK::FlushBarriers()
    {
        if (!mMemoryBarriers.empty() || !mBufferMemoryBarriers.empty() || !mImageMemoryBarriers.empty())
        {
            vk::DependencyInfo dependencyInfo {};
            dependencyInfo.setMemoryBarriers(mMemoryBarriers);
            dependencyInfo.setBufferMemoryBarriers(mBufferMemoryBarriers);
            dependencyInfo.setImageMemoryBarriers(mImageMemoryBarriers);

            mCmdBuffer.pipelineBarrier2(dependencyInfo);

            mMemoryBarriers.clear();
            mBufferMemoryBarriers.clear();
            mImageMemoryBarriers.clear();
        }
    }

    void RHICommandListVK::BeginRenderPass(const RHIRenderPassDesc &desc)
    {
        FlushBarriers();

        vk::RenderingAttachmentInfo colorAttachments[RHI_MAX_COLOR_ATTACHMENT_COUNT] {};
        vk::RenderingAttachmentInfo depthAttachment {};
        vk::RenderingAttachmentInfo stencilAttachment {};
        uint32_t width = 0, height = 0;

        for (uint32_t i = 0; i < RHI_MAX_COLOR_ATTACHMENT_COUNT; i++)
        {
            if (desc.Color[i].Texture)
            {
                if (width == 0)
                {
                    width = desc.Color[i].Texture->GetDesc().Width;
                }
                if (height == 0)
                {
                    height = desc.Color[i].Texture->GetDesc().Height;
                }
                assert(width == desc.Color[i].Texture->GetDesc().Width);
                assert(height == desc.Color[i].Texture->GetDesc().Height);

                colorAttachments[i].setImageView(((RHITextureVK*)desc.Color[i].Texture)->GetRenderView(desc.Color[i].MipSlice, desc.Color[i].ArraySlice));
                colorAttachments[i].setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
                colorAttachments[i].setLoadOp(GetLoadOp(desc.Color[i].LoadOp));
                colorAttachments[i].setStoreOp(GetStoreOp(desc.Color[i].StoreOp));
                memcpy(colorAttachments[i].clearValue.color.float32, desc.Color[i].ClearColor, sizeof(float) * 4);
            }
        }

        if (desc.Depth.Texture != nullptr)
        {
            if (width == 0)
            {
                width = desc.Depth.Texture->GetDesc().Width;
            }
            if (height == 0)
            {
                height = desc.Depth.Texture->GetDesc().Height;
            }
            assert(width == desc.Depth.Texture->GetDesc().Width);
            assert(height == desc.Depth.Texture->GetDesc().Height);

            depthAttachment.setImageView(((RHITextureVK*)desc.Depth.Texture)->GetRenderView(desc.Depth.MipSlice, desc.Depth.ArraySlice));
            depthAttachment.setImageLayout(desc.Depth.bReadOnly ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal);
            depthAttachment.setLoadOp(GetLoadOp(desc.Depth.DepthLoadOp));
            depthAttachment.setStoreOp(GetStoreOp(desc.Depth.DepthStoreOp));

            if (IsStencilFormat(desc.Depth.Texture->GetDesc().Format))
            {
                stencilAttachment.setImageView(((RHITextureVK*)desc.Depth.Texture)->GetRenderView(desc.Depth.MipSlice, desc.Depth.ArraySlice));
                stencilAttachment.setImageLayout(desc.Depth.bReadOnly ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal);
                stencilAttachment.setLoadOp(GetLoadOp(desc.Depth.StencilLoadOp));
                stencilAttachment.setStoreOp(GetStoreOp(desc.Depth.StencilStoreOp));
            }
        }

        vk::RenderingInfo renderInfo {};
        renderInfo.renderArea.extent.width = width;
        renderInfo.renderArea.extent.height = height;
        renderInfo.setLayerCount(1);
        renderInfo.setViewMask(0);
        renderInfo.setColorAttachments(colorAttachments);

        if (depthAttachment.imageView != VK_NULL_HANDLE)
        {
            renderInfo.pDepthAttachment = &depthAttachment;
        }
        if (stencilAttachment.imageView != VK_NULL_HANDLE)
        {
            renderInfo.pStencilAttachment = &stencilAttachment;
        }
        mCmdBuffer.beginRendering(renderInfo);

        SetViewport(0, 0, width, height);
    }

    void RHICommandListVK::EndRenderPass()
    {
        mCmdBuffer.endRendering();
    }

    void RHICommandListVK::SetPipelineState(RHIPipelineState *pipelineState)
    {
        vk::PipelineBindPoint bindPoint = pipelineState->GetType() == ERHIPipelineType::Compute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics;
        mCmdBuffer.bindPipeline(bindPoint, (VkPipeline)pipelineState->GetNativeHandle());
    }

    void RHICommandListVK::SetStencilReference(uint8_t stencil)
    {
        mCmdBuffer.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, stencil);
    }

    void RHICommandListVK::SetBlendFactor(const float *blendFactor)
    {
        mCmdBuffer.setBlendConstants(blendFactor);
    }

    void RHICommandListVK::SetIndexBuffer(RHIBuffer *buffer, uint32_t offset, ERHIFormat format)
    {
        vk::IndexType type = format == ERHIFormat::R16UI ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
        mCmdBuffer.bindIndexBuffer((VkBuffer)buffer->GetNativeHandle(), offset, type);
    }

    void RHICommandListVK::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        vk::Viewport viewport {};
        viewport.x = x;
        viewport.y = (float)height - (float)y;
        viewport.width = width;
        viewport.height = -(float)height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        mCmdBuffer.setViewport(0, 1, &viewport);
        SetScissorRect(x, y, width, height);
    }

    void RHICommandListVK::SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        vk::Rect2D scissor {};
        scissor.offset.x = x;
        scissor.offset.y = y;
        scissor.extent.width = width;
        scissor.extent.height = height;

        mCmdBuffer.setScissor(0, 1, &scissor);
    }

    void RHICommandListVK::SetGraphicsConstants(uint32_t slot, const void *data, size_t dataSize)
    {
        if (slot == 0)
        {
            assert(dataSize <= RHI_MAX_ROOT_CONSTANTS * sizeof(uint32_t));
            memcpy(mGraphicsConstants.cb0, data, dataSize);
        }
        else
        {
            assert(slot < RHI_MAX_CBV_BINDING);
            vk::DeviceAddress gpuAddress = ((RHIDeviceVK*)mpDevice)->AllocateConstantBuffer(data, dataSize);
            if (slot == 1)
            {
                mGraphicsConstants.cbv1.address = gpuAddress;
                mGraphicsConstants.cbv1.range = dataSize;
            }
            else
            {
                mGraphicsConstants.cbv2.address = gpuAddress;
                mGraphicsConstants.cbv2.range = dataSize;
            }
        }
    }

    void RHICommandListVK::SetComputeConstants(uint32_t slot, const void *data, size_t dataSize)
    {
        if (slot == 0)
        {
            assert(dataSize <= RHI_MAX_ROOT_CONSTANTS * sizeof(uint32_t));
            memcpy(mComputeConstants.cb0, data, dataSize);
        }
        else
        {
            assert(slot < RHI_MAX_CBV_BINDING);
            vk::DeviceAddress gpuAddress = ((RHIDeviceVK*)mpDevice)->AllocateConstantBuffer(data, dataSize);
            if (slot == 1)
            {
                mComputeConstants.cbv1.address = gpuAddress;
                mComputeConstants.cbv1.range = dataSize;
            }
            else
            {
                mComputeConstants.cbv2.address = gpuAddress;
                mComputeConstants.cbv2.range = dataSize;
            }
        }
    }

    void RHICommandListVK::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        UpdateGraphicsDescriptorBuffer();
        mCmdBuffer.draw(vertexCount, instanceCount, 0, 0);
    }

    void RHICommandListVK::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset)
    {
        UpdateGraphicsDescriptorBuffer();
        mCmdBuffer.drawIndexed(indexCount, instanceCount, indexOffset, 0, 0);
    }

    void RHICommandListVK::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        FlushBarriers();
        UpdateComputeDescriptorBuffer();
        mCmdBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void RHICommandListVK::DispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        UpdateGraphicsDescriptorBuffer();
        auto device = (RHIDeviceVK*)mpDevice;
        auto dynamicLoader = device->GetDynamicLoader();
        mCmdBuffer.drawMeshTasksEXT(groupCountX, groupCountY, groupCountZ, dynamicLoader);
    }

    void RHICommandListVK::DrawIndirect(RHIBuffer *buffer, uint32_t offset)
    {
        UpdateGraphicsDescriptorBuffer();
        mCmdBuffer.drawIndirect((VkBuffer)buffer->GetNativeHandle(), offset, 1, 0);
    }

    void RHICommandListVK::DrawIndexedIndirect(RHIBuffer *buffer, uint32_t offset)
    {
        UpdateGraphicsDescriptorBuffer();
        mCmdBuffer.drawIndexedIndirect((VkBuffer)buffer->GetNativeHandle(), offset, 1, 0);
    }

    void RHICommandListVK::DispatchIndirect(RHIBuffer *buffer, uint32_t offset)
    {
        FlushBarriers();
        UpdateComputeDescriptorBuffer();
        mCmdBuffer.dispatchIndirect((VkBuffer)buffer->GetNativeHandle(), offset);
    }

    void RHICommandListVK::UpdateGraphicsDescriptorBuffer()
    {
        auto device = (RHIDeviceVK*)mpDevice;
        auto dynamicLoader = device->GetDynamicLoader();
        vk::DeviceSize cbvDescOffset = device->AllocateConstantBufferDescriptor(mGraphicsConstants.cb0, mGraphicsConstants.cbv1, mGraphicsConstants.cbv2);

        uint32_t bufferIndices[] = {0, 1, 2};
        vk::DeviceSize offsets[] = { cbvDescOffset, 0, 0 };
        mCmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eGraphics, device->GetPipelineLayout(), 0, 3, bufferIndices, offsets, dynamicLoader);
    }

    void RHICommandListVK::UpdateComputeDescriptorBuffer()
    {
        auto device = (RHIDeviceVK*)mpDevice;
        auto dynamicLoader = device->GetDynamicLoader();
        vk::DeviceSize cbvDescOffset = device->AllocateConstantBufferDescriptor(mComputeConstants.cb0, mComputeConstants.cbv1, mComputeConstants.cbv2);

        uint32_t bufferIndices[] = {0, 1, 2};
        vk::DeviceSize offsets[] = { cbvDescOffset, 0, 0 };
        mCmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eCompute, device->GetPipelineLayout(), 0, 3, bufferIndices, offsets, dynamicLoader);
    }
}