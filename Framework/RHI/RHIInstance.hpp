#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    class RHIGPU;
    
    class RHIInstance
    {
    public:
        static RHIInstance* GetInstanceByRHIBackend(const RHIRenderBackend& backend);

        NOCOPY(RHIInstance)
        virtual ~RHIInstance() = default;

        virtual RHIRenderBackend GetRHIBackend() = 0;
        virtual uint32_t GetGPUCount() = 0;
        virtual RHIGPU* GetGPU(uint32_t index) = 0;
        virtual void Destroy() = 0;

    protected:
        explicit RHIInstance() = default;
    };
}