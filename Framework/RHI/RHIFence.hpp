#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIFence : public RHIResource
    {
    public:
        virtual ~RHIFence() = default;

        virtual void Waite(uint64_t value) = 0;
        virtual void Signal(uint64_t value) = 0;
    };
}