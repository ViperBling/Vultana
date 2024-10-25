#pragma once

#include "RHI/RHI.hpp"

#include <EASTL/unique_ptr.h>

namespace Renderer
{
    class RendererBase;
}

namespace RenderResources
{
    class StructuredBuffer
    {
    public:
        StructuredBuffer(const eastl::string& name);

        bool Create(uint32_t stride, uint32_t elementCount, RHI::ERHIMemoryType memoryType, bool isUAV);

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