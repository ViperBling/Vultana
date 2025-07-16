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

        RHI::RHIBuffer* GetBuffer() const { return m_pBuffer.get(); }
        RHI::RHIDescriptor* GetSRV() const { return m_pSRV.get(); }
        RHI::RHIDescriptor* GetUAV() const { return m_pUAV.get(); }

    protected:
        eastl::string m_Name;
        eastl::unique_ptr<RHI::RHIBuffer> m_pBuffer;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pSRV;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pUAV;
    };
}