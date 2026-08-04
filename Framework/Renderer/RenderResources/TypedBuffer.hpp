#pragma once

#include "RHI/RHI.hpp"
#include "EASTL/unique_ptr.h"

namespace RenderResources
{
    class FTypedBuffer
    {
    public:
        FTypedBuffer(const eastl::string& name) : m_Name(name) {}

        bool Create(RHI::ERHIFormat format, uint32_t elementCount, RHI::ERHIMemoryType memType, bool isUAV);

        RHI::FRHIBuffer* GetBuffer() const { return m_pBuffer.get(); }
        RHI::FRHIDescriptor* GetSRV() const { return m_pSRV.get(); }
        RHI::FRHIDescriptor* GetUAV() const { return m_pUAV.get(); }

    private:
        eastl::string m_Name;
        eastl::unique_ptr<RHI::FRHIBuffer> m_pBuffer;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pSRV;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pUAV;
    };
}