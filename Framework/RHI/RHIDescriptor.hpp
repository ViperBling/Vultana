#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class FRHIDescriptor : public FRHIResource
    {
    public:
        virtual uint32_t GetHeapIndex() const = 0;
    };
}