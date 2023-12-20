#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    struct QueueInfo
    {
        RHICommandQueueType Type;
        uint8_t Count;
    };

    struct DeviceCreateInfo
    {
        uint32_t QueueCreateInfoCount;
        const QueueInfo* QueueCreateInfos;
    };

    class RHIDevice
    {
    public:
        NOCOPY(RHIDevice)
        virtual ~RHIDevice() = default;

        virtual void Destroy() = 0;


    };
} // namespace Vultana::RHI
