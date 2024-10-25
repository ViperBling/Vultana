#pragma once

#include "RHI/RHI.hpp"

#include <EASTL/unique_ptr.h>

namespace RenderResources
{
    class RawBuffer
    {
    public:
        RawBuffer(const eastl::string& name);

        bool Create(uint32_t size, RHI::ERHIMemoryType memoryType, bool isUAV);

        RHI::RHIBuffer* GetBuffer() const { return mpBuffer.get(); }
        RHI::RHIDescriptor* GetSRV() const { return mpSRV.get(); }
        RHI::RHIDescriptor* GetUAV() const { return mpUAV.get(); }
    
    protected:
        eastl::string mName;
        eastl::unique_ptr<RHI::RHIBuffer> mpBuffer;
        eastl::unique_ptr<RHI::RHIDescriptor> mpSRV;
        eastl::unique_ptr<RHI::RHIDescriptor> mpUAV;
    };
}