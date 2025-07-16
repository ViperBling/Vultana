#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIPipelineState : public RHIResource
    {
    public:
        virtual ~RHIPipelineState() = default;
        
        virtual bool Create() = 0;
        
        ERHIPipelineType GetType() const { return m_Type; }

    protected:
        ERHIPipelineType m_Type;
    };
}