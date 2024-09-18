#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIHeap : public RHIResource
    {
    public:
        const RHIHeapDesc& GetDesc() const { return mDesc; }
    protected:
        RHIHeapDesc mDesc {};
    };
}