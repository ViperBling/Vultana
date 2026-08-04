#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class FRHIFence : public FRHIResource
    {
    public:
        virtual ~FRHIFence() = default;

        virtual void Wait(uint64_t value) = 0;
        virtual void Signal(uint64_t value) = 0;
    };
}