#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    class RHIDevice
    {
    public:
        virtual ~RHIDevice() {}
        virtual void* GetHandle() const = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual bool Init() = 0;
    };

    RHIDevice* CreateRHIDevice(const RHIDeviceInfo& deviceInfo);
} // namespace Vultana::RHI
