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
    RHICommandListVK::RHICommandListVK(RHIDeviceVK *device, ERHICommandQueueType queueType, const eastl::string &name)
    {
        m_pDevice = device;
        m_CmdQueueType = queueType;
        m_Name = name;
    }

    RHICommandListVK::~RHICommandListVK()
    {
        ((RHIDeviceVK*)m_pDevice)->Delete(m_CmdPool);
    }

    bool RHICommandListVK::Create()
    {
        auto device = (RHIDeviceVK*)m_pDevice;

        vk::CommandPoolCreateInfo cmdPoolCI {};

        switch (m_CmdQueueType)
        {
        case ERHICommandQueueType::Graphics:
            cmdPoolCI.setQueueFamilyIndex(device->GetGraphicsQueueIndex());
            m_Queue = device->GetGraphicsQueue();
            break;
        case ERHICommandQueueType::Compute:
            cmdPoolCI.setQueueFamilyIndex(device->GetComputeQueueIndex());
            m_Queue = device->GetComputeQueue();
            break;
        case ERHICommandQueueType::Copy:
            cmdPoolCI.setQueueFamilyIndex(device->GetCopyQueueIndex());
            m_Queue = device->GetCopyQueue();
            break;
        default:
            break;
        }

        vk::Device deviceHandle = device->GetDevice();
        m_DynamicLoader = device->GetDynamicLoader();
        vk::Result res = deviceHandle.createCommandPool(&cmdPoolCI, nullptr, &m_CmdPool);
        if (res != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHICommandListVK] Failed to create command pool");
            return false;
        }
        SetDebugName(deviceHandle, vk::ObjectType::eCommandPool, (uint64_t)(VkCommandPool)m_CmdPool, m_Name.c_str(), m_DynamicLoader);

        return true;
    }

    void RHICommandListVK::ResetAllocator()
    {
        vk::Device deviceHandle = ((RHIDeviceVK*)m_pDevice)->GetDevice();
        deviceHandle.resetCommandPool(m_CmdPool);

        for (size_t i = 0; i < m_PendingCmdBuffers.size(); i++)
        {
            m_FreeCmdBuffers.push_back(m_PendingCmdBuffers[i]);
        }
        m_PendingCmdBuffers.clear();
    }

    void RHICommandListVK::Begin()
    {
        vk::Result res;

        if (!m_FreeCmdBuffers.empty())
        {
            m_CmdBuffer = m_FreeCmdBuffers.back();
            m_FreeCmdBuffers.pop_back();
        }
        else
        {
            vk::CommandBufferAllocateInfo cmdBufferAI {};
            cmdBufferAI.setCommandPool(m_CmdPool);
            cmdBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
            cmdBufferAI.setCommandBufferCount(1);

            vk::Device deviceHandle = ((RHIDeviceVK*)m_pDevice)->GetDevice();
            res = deviceHandle.allocateCommandBuffers(&cmdBufferAI, &m_CmdBuffer);
            if (res != vk::Result::eSuccess)
            {
                VTNA_LOG_ERROR("[RHICommandListVK] Failed to allocate command buffer");
                return;
            }
        }
        vk::CommandBufferBeginInfo cmdBufferBI {};
        cmdBufferBI.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        m_CmdBuffer.begin(cmdBufferBI);

        ResetState();
    }

    void RHICommandListVK::End()
    {
        FlushBarriers();

        m_CmdBuffer.end();
        m_PendingCmdBuffers.push_back(m_CmdBuffer);
    }

    void RHICommandListVK::Wait(RHIFence *fence, uint64_t value)
    {
        m_PendingWaits.emplace_back(fence, value);
    }

    void RHICommandListVK::Signal(RHIFence *fence, uint64_t value)
    {
        m_PendingSignals.emplace_back(fence, value);
    }

    void RHICommandListVK::Present(RHISwapchain *swapchain)
    {
        m_PendingSwapchain.push_back(swapchain);
    }

    void RHICommandListVK::Submit()
    {
        ((RHIDeviceVK*)m_pDevice)->FlushLayoutTransition(m_CmdQueueType);

        eastl::vector<vk::Semaphore> waitSemaphores;
        eastl::vector<vk::Semaphore> signalSemaphores;
        eastl::vector<uint64_t> waitValues;
        eastl::vector<uint64_t> signalValues;
        eastl::vector<vk::PipelineStageFlags> waitStages;

        for (size_t i = 0; i < m_PendingWaits.size(); i++)
        {
            waitSemaphores.push_back((VkSemaphore)m_PendingWaits[i].first->GetNativeHandle());
            waitStages.push_back(vk::PipelineStageFlagBits::eTopOfPipe);
            waitValues.push_back(m_PendingWaits[i].second);
        }
        m_PendingWaits.clear();

        for (size_t i = 0; i < m_PendingSignals.size(); i++)
        {
            signalSemaphores.push_back((VkSemaphore)m_PendingSignals[i].first->GetNativeHandle());
            signalValues.push_back(m_PendingSignals[i].second);
        }
        m_PendingSignals.clear();

        for (size_t i = 0; i < m_PendingSwapchain.size(); i++)
        {
            auto swapchain = (RHISwapchainVK*)m_PendingSwapchain[i];
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
        submitInfo.setCommandBuffers(m_CmdBuffer);

        m_Queue.submit(submitInfo);

        for (size_t i = 0; i < m_PendingSwapchain.size(); i++)
        {
            auto swapchain = (RHISwapchainVK*)m_PendingSwapchain[i];
            swapchain->Present(m_Queue);
        }
        m_PendingSwapchain.clear();
    }
    
    void RHICommandListVK::ResetState()
    {
        if (m_CmdQueueType == ERHICommandQueueType::Graphics || m_CmdQueueType == ERHICommandQueueType::Compute)
        {
            auto device = (RHIDeviceVK*)m_pDevice;

            vk::DescriptorBufferBindingInfoEXT descBufferBI[3] {};
            descBufferBI[0].setAddress(device->GetConstantBufferAllocator()->GetGPUAddress());
            descBufferBI[0].setUsage(vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
            descBufferBI[1].setAddress(device->GetResourceDescriptorAllocator()->GetGPUAddress());
            descBufferBI[1].setUsage(vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
            descBufferBI[2].setAddress(device->GetSamplerDescriptorAllocator()->GetGPUAddress());
            descBufferBI[2].setUsage(vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT);

            m_CmdBuffer.bindDescriptorBuffersEXT(3, descBufferBI, m_DynamicLoader);

            uint32_t bufferIndices[] = { 1, 2 };
            vk::DeviceSize offsets[] = { 0, 0 };

            m_CmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eCompute, device->GetPipelineLayout(), 1, 2, bufferIndices, offsets, m_DynamicLoader);

            if (m_CmdQueueType == ERHICommandQueueType::Graphics)
            {
                m_CmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eGraphics, device->GetPipelineLayout(), 1, 2, bufferIndices, offsets, m_DynamicLoader);
            }
        }
    }

    void RHICommandListVK::BeginProfiling()
    {
    }

    void RHICommandListVK::EndProfiling()
    {
    }

    void RHICommandListVK::BeginEvent(const eastl::string &eventName)
    {
        vk::DebugUtilsLabelEXT label {};
        label.pLabelName = eventName.c_str();

        m_CmdBuffer.beginDebugUtilsLabelEXT(label, m_DynamicLoader);
    }

    void RHICommandListVK::EndEvent()
    {
        m_CmdBuffer.endDebugUtilsLabelEXT(m_DynamicLoader);
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
        copy2.imageExtent.width = eastl::max(desc.Width >> mipLevel, 1u);
        copy2.imageExtent.height = eastl::max(desc.Height >> mipLevel, 1u);
        copy2.imageExtent.depth = eastl::max(desc.Depth >> mipLevel, 1u);

        vk::CopyBufferToImageInfo2 copyInfo {};
        copyInfo.srcBuffer = (VkBuffer)srcBuffer->GetNativeHandle();
        copyInfo.dstImage = (VkImage)dstTexture->GetNativeHandle();
        copyInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &copy2;

        m_CmdBuffer.copyBufferToImage2(copyInfo);
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
        copy2.imageExtent.width = eastl::max(desc.Width >> mipLevel, 1u);
        copy2.imageExtent.height = eastl::max(desc.Height >> mipLevel, 1u);
        copy2.imageExtent.depth = eastl::max(desc.Depth >> mipLevel, 1u);

        vk::CopyImageToBufferInfo2 copyInfo {};
        copyInfo.srcImage = (VkImage)srcTexture->GetNativeHandle();
        copyInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
        copyInfo.dstBuffer = (VkBuffer)dstBuffer->GetNativeHandle();
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &copy2;

        m_CmdBuffer.copyImageToBuffer2(copyInfo);
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

        m_CmdBuffer.copyBuffer2(copyInfo2);
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
        copy2.extent.width = eastl::max(src->GetDesc().Width >> srcMipLevel, 1u);
        copy2.extent.height = eastl::max(src->GetDesc().Height >> srcMipLevel, 1u);
        copy2.extent.depth = eastl::max(src->GetDesc().Depth >> srcMipLevel, 1u);

        vk::CopyImageInfo2 copyInfo2 {};
        copyInfo2.setSrcImage((VkImage)src->GetNativeHandle());
        copyInfo2.setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal);
        copyInfo2.setDstImage((VkImage)dst->GetNativeHandle());
        copyInfo2.setDstImageLayout(vk::ImageLayout::eTransferDstOptimal);
        copyInfo2.regionCount = 1;
        copyInfo2.pRegions = &copy2;

        m_CmdBuffer.copyImage2(copyInfo2);
    }

    void RHICommandListVK::ClearUAV(RHIResource *resource, RHIDescriptor *uav, const float *clearValue)
    {
        ConstantData constantData = m_ComputeConstants;

        const RHI::RHIUnorderedAccessViewDesc& uavDesc = static_cast<RHI::RHIUnorderedAccessViewVK*>(uav)->GetDesc();
        Renderer::ClearUAV(this, resource, uav, uavDesc, clearValue);

        m_ComputeConstants = constantData;
        m_ComputeConstants.dirty = true;
    }

    void RHICommandListVK::ClearUAV(RHIResource *resource, RHIDescriptor *uav, const uint32_t *clearValue)
    {
        ConstantData constantData = m_ComputeConstants;

        const RHI::RHIUnorderedAccessViewDesc& uavDesc = static_cast<RHI::RHIUnorderedAccessViewVK*>(uav)->GetDesc();
        Renderer::ClearUAV(this, resource, uav, uavDesc, clearValue);

        m_ComputeConstants = constantData;
        m_ComputeConstants.dirty = true;
    }

    void RHICommandListVK::WriteBuffer(RHIBuffer *buffer, uint32_t offset, uint32_t data)
    {
        FlushBarriers();

        m_CmdBuffer.updateBuffer((VkBuffer)buffer->GetNativeHandle(), offset, sizeof(uint32_t), &data);
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

        m_ImageMemoryBarriers.push_back(barrier);
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

        m_BufferMemoryBarriers.push_back(barrier2);
    }

    void RHICommandListVK::GlobalBarrier(ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter)
    {
        vk::MemoryBarrier2 barrier2 {};
        barrier2.setSrcStageMask(GetStageMask(accessFlagBefore));
        barrier2.setDstStageMask(GetStageMask(accessFlagAfter));
        barrier2.setSrcAccessMask(GetAccessMask(accessFlagBefore));
        barrier2.setDstAccessMask(GetAccessMask(accessFlagAfter));

        m_MemoryBarriers.push_back(barrier2);
    }

    void RHICommandListVK::FlushBarriers()
    {
        if (!m_MemoryBarriers.empty() || !m_BufferMemoryBarriers.empty() || !m_ImageMemoryBarriers.empty())
        {
            vk::DependencyInfo dependencyInfo {};
            dependencyInfo.setMemoryBarriers(m_MemoryBarriers);
            dependencyInfo.setBufferMemoryBarriers(m_BufferMemoryBarriers);
            dependencyInfo.setImageMemoryBarriers(m_ImageMemoryBarriers);

            m_CmdBuffer.pipelineBarrier2(dependencyInfo);

            m_MemoryBarriers.clear();
            m_BufferMemoryBarriers.clear();
            m_ImageMemoryBarriers.clear();
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
        m_CmdBuffer.beginRendering(renderInfo);

        SetViewport(0, 0, width, height);
    }

    void RHICommandListVK::EndRenderPass()
    {
        m_CmdBuffer.endRendering();
    }

    void RHICommandListVK::SetPipelineState(RHIPipelineState *pipelineState)
    {
        vk::PipelineBindPoint bindPoint = pipelineState->GetType() == ERHIPipelineType::Compute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics;
        m_CmdBuffer.bindPipeline(bindPoint, (VkPipeline)pipelineState->GetNativeHandle());
    }

    void RHICommandListVK::SetStencilReference(uint8_t stencil)
    {
        m_CmdBuffer.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, stencil);
    }

    void RHICommandListVK::SetBlendFactor(const float *blendFactor)
    {
        m_CmdBuffer.setBlendConstants(blendFactor);
    }

    void RHICommandListVK::SetIndexBuffer(RHIBuffer *buffer, uint32_t offset, ERHIFormat format)
    {
        vk::IndexType type = format == ERHIFormat::R16UI ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
        m_CmdBuffer.bindIndexBuffer((VkBuffer)buffer->GetNativeHandle(), offset, type);
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

        m_CmdBuffer.setViewport(0, 1, &viewport);
        SetScissorRect(x, y, width, height);
    }

    void RHICommandListVK::SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        vk::Rect2D scissor {};
        scissor.offset.x = x;
        scissor.offset.y = y;
        scissor.extent.width = width;
        scissor.extent.height = height;

        m_CmdBuffer.setScissor(0, 1, &scissor);
    }

    void RHICommandListVK::SetGraphicsConstants(uint32_t slot, const void *data, size_t dataSize)
    {
        if (slot == 0)
        {
            assert(dataSize <= RHI_MAX_ROOT_CONSTANTS * sizeof(uint32_t));
            memcpy(m_GraphicsConstants.cb0, data, dataSize);
        }
        else
        {
            assert(slot < RHI_MAX_CBV_BINDING);
            vk::DeviceAddress gpuAddress = ((RHIDeviceVK*)m_pDevice)->AllocateConstantBuffer(data, dataSize);
            if (slot == 1)
            {
                m_GraphicsConstants.cbv1.address = gpuAddress;
                m_GraphicsConstants.cbv1.range = dataSize;
            }
            else
            {
                m_GraphicsConstants.cbv2.address = gpuAddress;
                m_GraphicsConstants.cbv2.range = dataSize;
            }
        }
        m_GraphicsConstants.dirty = true;
    }

    void RHICommandListVK::SetComputeConstants(uint32_t slot, const void *data, size_t dataSize)
    {
        if (slot == 0)
        {
            assert(dataSize <= RHI_MAX_ROOT_CONSTANTS * sizeof(uint32_t));
            memcpy(m_ComputeConstants.cb0, data, dataSize);
        }
        else
        {
            assert(slot < RHI_MAX_CBV_BINDING);
            vk::DeviceAddress gpuAddress = ((RHIDeviceVK*)m_pDevice)->AllocateConstantBuffer(data, dataSize);
            if (slot == 1)
            {
                m_ComputeConstants.cbv1.address = gpuAddress;
                m_ComputeConstants.cbv1.range = dataSize;
            }
            else
            {
                m_ComputeConstants.cbv2.address = gpuAddress;
                m_ComputeConstants.cbv2.range = dataSize;
            }
        }
        m_ComputeConstants.dirty = true;
    }

    void RHICommandListVK::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        UpdateGraphicsDescriptorBuffer();
        m_CmdBuffer.draw(vertexCount, instanceCount, 0, 0);
    }

    void RHICommandListVK::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset)
    {
        UpdateGraphicsDescriptorBuffer();
        m_CmdBuffer.drawIndexed(indexCount, instanceCount, indexOffset, 0, 0);
    }

    void RHICommandListVK::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        FlushBarriers();
        UpdateComputeDescriptorBuffer();
        m_CmdBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void RHICommandListVK::DispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        UpdateGraphicsDescriptorBuffer();
        m_CmdBuffer.drawMeshTasksEXT(groupCountX, groupCountY, groupCountZ, m_DynamicLoader);
    }

    void RHICommandListVK::DrawIndirect(RHIBuffer *buffer, uint32_t offset)
    {
        UpdateGraphicsDescriptorBuffer();
        m_CmdBuffer.drawIndirect((VkBuffer)buffer->GetNativeHandle(), offset, 1, 0);
    }

    void RHICommandListVK::DrawIndexedIndirect(RHIBuffer *buffer, uint32_t offset)
    {
        UpdateGraphicsDescriptorBuffer();
        m_CmdBuffer.drawIndexedIndirect((VkBuffer)buffer->GetNativeHandle(), offset, 1, 0);
    }

    void RHICommandListVK::DispatchIndirect(RHIBuffer *buffer, uint32_t offset)
    {
        FlushBarriers();
        UpdateComputeDescriptorBuffer();
        m_CmdBuffer.dispatchIndirect((VkBuffer)buffer->GetNativeHandle(), offset);
    }

     void RHICommandListVK::DispatchMeshIndirect(RHIBuffer *buffer, uint32_t offset)
    {
        UpdateGraphicsDescriptorBuffer();

        m_CmdBuffer.drawMeshTasksIndirectEXT((VkBuffer)buffer->GetNativeHandle(), offset, 1, 0, m_DynamicLoader);
    }

    void RHICommandListVK::MultiDrawIndirect(uint32_t maxCount, RHIBuffer *argsBuffer, uint32_t argsBufferOffset, RHIBuffer *countBuffer, uint32_t countBufferOffset)
    {
        UpdateGraphicsDescriptorBuffer();

        m_CmdBuffer.drawIndirectCount((VkBuffer)argsBuffer->GetNativeHandle(), argsBufferOffset, (VkBuffer)countBuffer->GetNativeHandle(), countBufferOffset, maxCount, sizeof(RHIDrawCommand));
    }

    void RHICommandListVK::MultiDrawIndexedIndirect(uint32_t maxCount, RHIBuffer *argsBuffer, uint32_t argsBufferOffset, RHIBuffer *countBuffer, uint32_t countBufferOffset)
    {
        UpdateGraphicsDescriptorBuffer();

        m_CmdBuffer.drawIndexedIndirectCount((VkBuffer)argsBuffer->GetNativeHandle(), argsBufferOffset, (VkBuffer)countBuffer->GetNativeHandle(), countBufferOffset, maxCount, sizeof(RHIDrawIndexedCommand));
    }

    void RHICommandListVK::MultiDispatchIndirect(uint32_t maxCount, RHIBuffer *argsBuffer, uint32_t argsBufferOffset, RHIBuffer *countBuffer, uint32_t countBufferOffset)
    {
        FlushBarriers();
        UpdateComputeDescriptorBuffer();

        // Not Supported.
        assert(false);
    }

    void RHICommandListVK::MultiDispatchMeshIndirect(uint32_t maxCount, RHIBuffer *argsBuffer, uint32_t argsBufferOffset, RHIBuffer *countBuffer, uint32_t countBufferOffset)
    {
        UpdateGraphicsDescriptorBuffer();

        m_CmdBuffer.drawMeshTasksIndirectCountEXT((VkBuffer)argsBuffer->GetNativeHandle(), argsBufferOffset, (VkBuffer)countBuffer->GetNativeHandle(), countBufferOffset, maxCount, sizeof(RHIDispatchCommand), m_DynamicLoader);
    }

    void RHICommandListVK::UpdateGraphicsDescriptorBuffer()
    {
        if (!m_GraphicsConstants.dirty) return;

        auto device = (RHIDeviceVK*)m_pDevice;
        vk::DeviceSize cbvDescOffset = device->AllocateConstantBufferDescriptor(m_GraphicsConstants.cb0, m_GraphicsConstants.cbv1, m_GraphicsConstants.cbv2);

        uint32_t bufferIndices[] = { 0 };
        vk::DeviceSize offsets[] = { cbvDescOffset };
        m_CmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eGraphics, device->GetPipelineLayout(), 0, 1, bufferIndices, offsets, m_DynamicLoader);

        m_GraphicsConstants.dirty = false;
    }

    void RHICommandListVK::UpdateComputeDescriptorBuffer()
    {
        if (!m_GraphicsConstants.dirty) return;

        auto device = (RHIDeviceVK*)m_pDevice;
        vk::DeviceSize cbvDescOffset = device->AllocateConstantBufferDescriptor(m_ComputeConstants.cb0, m_ComputeConstants.cbv1, m_ComputeConstants.cbv2);

        uint32_t bufferIndices[] = { 0 };
        vk::DeviceSize offsets[] = { cbvDescOffset };
        m_CmdBuffer.setDescriptorBufferOffsetsEXT(vk::PipelineBindPoint::eCompute, device->GetPipelineLayout(), 0, 1, bufferIndices, offsets, m_DynamicLoader);

        m_ComputeConstants.dirty = false;
    }
}