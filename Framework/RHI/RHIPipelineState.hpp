#pragma once

#include "RHIResource.hpp"

namespace Vultana
{
    class RHIPipelineState : public RHIResource
    {
    public:
        RHIPipelineType GetType() const { return mType; }
        virtual bool Create() = 0;

    protected:
        RHIPipelineType mType;
    };
}