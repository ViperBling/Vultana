#pragma once

#include "RHI/RHIQueue.hpp"

#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class QueueVK : public RHIQueue
    {
    public:
        NOCOPY(QueueVK)
        explicit QueueVK(vk::Queue queue);
        ~QueueVK() override = default;

        void Submit(RHICommandBuffer* commandBuffer, RHIFence* fenceToSignal) override;
        void Wait(RHIFence* fenceToSignal) override;

        vk::Queue GetVkQueue() const { return mQueue; }

    private:
        vk::Queue mQueue;
    };
}