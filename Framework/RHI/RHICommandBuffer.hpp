#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    class RHICommandList;

    class RHICommandBuffer
    {
    public:
        NOCOPY(RHICommandBuffer)
        virtual ~RHICommandBuffer() = default;

        virtual RHICommandList* Begin() = 0;
        virtual void Destroy() = 0;

    protected:
        RHICommandBuffer() = default;
    };
}