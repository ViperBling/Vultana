#pragma once

#include "RHI/RHI.hpp"

#include <EASTL/unique_ptr.h>

namespace Renderer
{
    class FRendererBase;
}

namespace RenderResources
{
    class FStructuredBuffer
    {
    public:
        FStructuredBuffer(const eastl::string& name);

        bool Create(uint32_t stride, uint32_t elementCount, RHI::ERHIMemoryType memoryType, bool isUAV);

        RHI::FRHIBuffer* GetBuffer() const { return m_pBuffer.get(); }
        RHI::FRHIDescriptor* GetSRV() const { return m_pSRV.get(); }
        RHI::FRHIDescriptor* GetUAV() const { return m_pUAV.get(); }

    protected:
        eastl::string m_Name;
        eastl::unique_ptr<RHI::FRHIBuffer> m_pBuffer;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pSRV;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pUAV;
    };
}