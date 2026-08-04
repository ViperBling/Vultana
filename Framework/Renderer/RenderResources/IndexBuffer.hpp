#pragma once

#include "RHI/RHI.hpp"

#include <EASTL/unique_ptr.h>

namespace Renderer
{
    class FRendererBase;
}

namespace RenderResources
{
    class FIndexBuffer
    {
    public:
        FIndexBuffer(const eastl::string& name);

        bool Create(uint32_t stride, uint32_t indexCount, RHI::ERHIMemoryType memoryType);

        RHI::FRHIBuffer* GetBuffer() const { return m_pBuffer.get(); }
        uint32_t GetIndexCount() const { return m_IndexCount; }
        RHI::ERHIFormat GetFormat() const { return m_pBuffer->GetDesc().Format; }
    
    protected:
        eastl::string m_Name;

        eastl::unique_ptr<RHI::FRHIBuffer> m_pBuffer;
        uint32_t m_IndexCount = 0;
    };
}