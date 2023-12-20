#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    class RHICommandBuffer;
    class RHIFence;

    class RHIQueue
    {
    public:
        NOCOPY(RHIQueue)
        virtual ~RHIQueue() = default;

        virtual void Submit(RHICommandBuffer* commandBuffer, RHIFence* fenceToSignal) = 0;
        virtual void Wait(RHIFence* fenceToSignal) = 0;

    protected:
        RHIQueue() = default;
    };
}