#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class FRHIHeap : public FRHIResource
    {
    public:
        const FRHIHeapDesc& GetDesc() const { return m_Desc; }
    protected:
        FRHIHeapDesc m_Desc {};
    };
}