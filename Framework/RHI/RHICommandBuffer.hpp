#pragma once

#include "RHICommon.hpp"

namespace RHI
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