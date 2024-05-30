#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    class RHICommandBuffer;
    class RHIFence;
    class RHISemaphore;

    struct QueueSubmitInfo
    {
        std::vector<RHISemaphore*> WaitSemaphores;
        std::vector<RHISemaphore*> SignalSemaphores;
        RHIFence* FenceToSignal;

        QueueSubmitInfo()
            : FenceToSignal(nullptr)
        {}
        QueueSubmitInfo& AddWaitSemaphore(RHISemaphore* semaphore)
        {
            WaitSemaphores.emplace_back(semaphore);
            return *this;
        }
        QueueSubmitInfo& AddSignalSemaphore(RHISemaphore* semaphore)
        {
            SignalSemaphores.emplace_back(semaphore);
            return *this;
        }
        QueueSubmitInfo& SetFenceToSignal(RHIFence* fence)
        {
            FenceToSignal = fence;
            return *this;
        }
    };

    class RHIQueue
    {
    public:
        NOCOPY(RHIQueue)
        virtual ~RHIQueue() = default;

        virtual void Submit(RHICommandBuffer* commandBuffer, const QueueSubmitInfo& submitInfo) = 0;
        virtual void Flush(RHIFence* fenceToSignal) = 0;

    protected:
        RHIQueue() = default;
    };
}