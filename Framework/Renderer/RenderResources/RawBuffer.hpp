#pragma once

#include "RHI/RHI.hpp"

#include <memory>

namespace RenderResources
{
    class RawBuffer
    {
    public:
        RawBuffer(const std::string& name);

        bool Create(uint32_t size, RHI::ERHIMemoryType memoryType, bool isUAV);

        RHI::RHIBuffer* GetBuffer() const { return mpBuffer.get(); }
        RHI::RHIDescriptor* GetSRV() const { return mpSRV.get(); }
        RHI::RHIDescriptor* GetUAV() const { return mpUAV.get(); }
    
    protected:
        std::string mName;
        std::unique_ptr<RHI::RHIBuffer> mpBuffer;
        std::unique_ptr<RHI::RHIDescriptor> mpSRV;
        std::unique_ptr<RHI::RHIDescriptor> mpUAV;
    };
}