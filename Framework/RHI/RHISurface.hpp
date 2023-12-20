#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    struct SurfaceCreateInfo
    {
        void* Window;
    };

    class RHISurface
    {
    public:
        NOCOPY(RHISurface)
        virtual ~RHISurface() = default;

        virtual void Destroy() = 0;
    
    protected:
        explicit RHISurface(const SurfaceCreateInfo& CreateInfo) {}
    };
}