#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    class RHIDevice;
    struct DeviceCreateInfo;

    struct GPUProperty
    {
        uint32_t VendorID;
        uint32_t DeviceID;
        RHIDeviceType Type;
    };

    class RHIGPU
    {
    public:
        NOCOPY(RHIGPU)
        virtual ~RHIGPU() = default;

        virtual GPUProperty GetProperty() = 0;
        virtual RHIDevice* RequestDevice(const DeviceCreateInfo& info) = 0;

    protected:
        RHIGPU() = default;
    };
}