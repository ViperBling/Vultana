#pragma once

#include "RHI/RHI.hpp"
#include "EASTL/unique_ptr.h"

namespace RenderResources
{
    class TypedBuffer
    {
    public:
        TypedBuffer(const eastl::string& name) : m_Name(name) {}

        bool Create(RHI::ERHIFormat format, uint32_t elementCount, RHI::ERHIMemoryType memType, bool isUAV);

        RHI::RHIBuffer* GetBuffer() const { return m_pBuffer.get(); }
        RHI::RHIDescriptor* GetSRV() const { return m_pSRV.get(); }
        RHI::RHIDescriptor* GetUAV() const { return m_pUAV.get(); }

    private:
        eastl::string m_Name;
        eastl::unique_ptr<RHI::RHIBuffer> m_pBuffer;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pSRV;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pUAV;
    };
}