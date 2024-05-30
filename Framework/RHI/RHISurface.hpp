#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    struct SurfaceCreateInfo
    {
        void* Window;

        SurfaceCreateInfo()
            : Window(nullptr)
        {}
        SurfaceCreateInfo& SetWindow(void* inWindow)
        {
            Window = inWindow;
            return *this;
        }
    };

    class RHISurface
    {
    public:
        NOCOPY(RHISurface)
        virtual ~RHISurface() = default;
    
    protected:
        explicit RHISurface(const SurfaceCreateInfo& CreateInfo) {}
    };
}