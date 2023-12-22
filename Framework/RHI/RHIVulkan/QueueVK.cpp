#include "QueueVK.hpp"

namespace Vultana
{
    QueueVK::QueueVK(vk::Queue queue)
        : RHIQueue()
        , mQueue(queue)
    {
    }

    void QueueVK::Submit(RHICommandBuffer *commandBuffer, RHIFence *fenceToSignal)
    {
    }
    
    void QueueVK::Wait(RHIFence *fenceToSignal)
    {
    }
}