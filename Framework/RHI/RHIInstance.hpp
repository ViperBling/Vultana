#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    class RHIInstance
    {
    public:
        static RHIInstance* GetInstanceByRHIBackend(const RHIRenderBackend& backend);

        NOCOPY(RHIInstance)
        virtual ~RHIInstance() = default;

        virtual RHIRenderBackend GetRHIBackend() = 0;
        virtual void Destroy() = 0;

    protected:
        explicit RHIInstance() = default;
    };
}