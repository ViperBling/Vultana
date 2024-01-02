#include "QueueVK.hpp"

#include "CommandBufferVK.hpp"
#include "SynchronousVK.hpp"

namespace RHI
{
    QueueVK::QueueVK(vk::Queue queue)
        : RHIQueue()
        , mQueue(queue)
    {
    }

    void QueueVK::Submit(RHICommandBuffer *commandBuffer, RHIFence *fenceToSignal)
    {
        auto* cmdBuffer = dynamic_cast<CommandBufferVK*>(commandBuffer);
        auto* fence = dynamic_cast<FenceVK*>(fenceToSignal);
        assert(cmdBuffer);
        
        const vk::CommandBuffer& cmdBufferVK = cmdBuffer->GetVkCommandBuffer();
        const vk::Fence fenceVK = fence ? fence->GetVkFence() : VK_NULL_HANDLE;

        vk::SubmitInfo submitInfo {};
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&cmdBufferVK);
        submitInfo.setWaitSemaphores(cmdBuffer->GetWaitSemaphores());
        submitInfo.setSignalSemaphores(cmdBuffer->GetSignalSemaphores());
        submitInfo.pWaitDstStageMask = cmdBuffer->GetWaitStages().data();

        if (fence) { fence->Reset(); }
        mQueue.submit(submitInfo, fenceVK);
    }
    
    void QueueVK::Wait(RHIFence *fenceToSignal)
    {
        
    }
}