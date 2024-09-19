#pragma once

#include "RHI/RHI.hpp"

#include <memory>

namespace Renderer
{
    class StructuredBuffer
    {
    public:
        StructuredBuffer(const std::string& name);

        bool Create(uint32_t stride, uint32_t elementCount, RHI::ERHIMemoryType memoryType, bool isUAV);

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