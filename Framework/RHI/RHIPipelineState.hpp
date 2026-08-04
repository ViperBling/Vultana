#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class FRHIPipelineState : public FRHIResource
    {
    public:
        virtual ~FRHIPipelineState() = default;
        
        virtual bool Create() = 0;
        
        ERHIPipelineType GetType() const { return m_Type; }

    protected:
        ERHIPipelineType m_Type;
    };
}