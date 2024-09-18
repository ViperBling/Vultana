#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIDescriptor : public RHIResource
    {
    public:
        virtual uint32_t GetHeapIndex() const = 0;
    };
}