#pragma once

#include "RHIResource.hpp"

namespace Vultana
{
    class RHIFence : public RHIResource
    {
    public:
        virtual ~RHIFence() = default;

        virtual void Wait(uint64_t value) = 0;
        virtual void Signal(uint64_t value) = 0;
    };
}