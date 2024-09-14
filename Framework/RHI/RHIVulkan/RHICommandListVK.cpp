#include "RHICommandListVK.hpp"
#include "RHIDescriptorAllocatorVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIHeapVK.hpp"
#include "RHIFenceVK.hpp"
#include "RHISwapchainVK.hpp"
#include "RHITextureVK.hpp"

#include "RHI/RHI.hpp"

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

        res = mCmdBuffer.begin(&cmdBufferBI);
        if (res != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHICommandListVK] Failed to begin command buffer");
            return;
        }

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

    void RHICommandListVK::CopyBufferToTexture(RHIBuffer *srcBuffer, RHITexture *dstTexture, uint32_t mipLevel, uint32_t arraySlice, uint32_t offset)
    {
    }

    void RHICommandListVK::CopyTextureToBuffer(RHITexture *srcTexture, RHIBuffer *dstBuffer, uint32_t mipLevel, uint32_t arraySlice)
    {
    }

    void RHICommandListVK::CopyBuffer(RHIBuffer *src, RHIBuffer *dst, uint32_t srcOffset, uint32_t dstOffset, uint32_t size)
    {
    }

    void RHICommandListVK::CopyTexture(RHITexture *src, RHITexture *dst, uint32_t srcMipLevel, uint32_t dstMipLevel, uint32_t srcArraySlice, uint32_t dstArraySlice)
    {
    }

    void RHICommandListVK::ClearUAV(RHIResource *resource, RHIDescriptor *uav, const float *clearValue)
    {
    }

    void RHICommandListVK::ClearUAV(RHIResource *resource, RHIDescriptor *uav, const uint32_t *clearValue)
    {
    }

    void RHICommandListVK::WriteBuffer(RHIBuffer *buffer, uint32_t offset, uint32_t size, const void *data)
    {
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
    }

    void RHICommandListVK::GlobalBarrier(ERHIAccessFlags accessFlagBefore, ERHIAccessFlags accessFlagAfter)
    {
    }

    void RHICommandListVK::FlushBarriers()
    {
    }

    void RHICommandListVK::BeginRenderPass(const RHIRenderPassDesc &desc)
    {
    }

    void RHICommandListVK::EndRenderPass()
    {
    }

    void RHICommandListVK::SetPipelineState(RHIPipelineState *pipelineState)
    {
    }

    void RHICommandListVK::SetStencilReference(uint8_t stencil)
    {
    }

    void RHICommandListVK::SetBlendFactor(const float *blendFactor)
    {
    }

    void RHICommandListVK::SetIndexBuffer(RHIBuffer *buffer, uint32_t offset, ERHIFormat format)
    {
    }

    void RHICommandListVK::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
    }

    void RHICommandListVK::SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
    }

    void RHICommandListVK::SetGraphicsConstants(uint32_t slot, const void *data, size_t dataSize)
    {
    }

    void RHICommandListVK::SetComputeConstants(uint32_t slot, const void *data, size_t dataSize)
    {
    }

    void RHICommandListVK::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
    }

    void RHICommandListVK::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset)
    {
    }

    void RHICommandListVK::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
    }

    void RHICommandListVK::DispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
    }

    void RHICommandListVK::DrawIndirect(RHIBuffer *buffer, uint32_t offset)
    {
    }

    void RHICommandListVK::DrawIndexedIndirect(RHIBuffer *buffer, uint32_t offset)
    {
    }

    void RHICommandListVK::DispatchIndirect(RHIBuffer *buffer, uint32_t offset)
    {
    }

    void RHICommandListVK::UpdateGraphicsDescriptorBuffer()
    {
    }

    void RHICommandListVK::UpdateComputeDescriptorBuffer()
    {
    }
}