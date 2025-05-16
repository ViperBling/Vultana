#pragma once

#include "RHI/RHI.hpp"
#include "EASTL/unique_ptr.h"

namespace RenderResources
{
    class TypedBuffer
    {
    public:
        TypedBuffer(const eastl::string& name) : mName(name) {}

        bool Create(RHI::ERHIFormat format, uint32_t elementCount, RHI::ERHIMemoryType memType, bool isUAV);

        RHI::RHIBuffer* GetBuffer() const { return mpBuffer.get(); }
        RHI::RHIDescriptor* GetSRV() const { return mpSRV.get(); }
        RHI::RHIDescriptor* GetUAV() const { return mpUAV.get(); }

    private:
        eastl::string mName;
        eastl::unique_ptr<RHI::RHIBuffer> mpBuffer;
        eastl::unique_ptr<RHI::RHIDescriptor> mpSRV;
        eastl::unique_ptr<RHI::RHIDescriptor> mpUAV;
    };
}