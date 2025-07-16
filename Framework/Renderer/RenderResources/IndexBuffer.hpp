#pragma once

#include "RHI/RHI.hpp"

#include <EASTL/unique_ptr.h>

namespace Renderer
{
    class RendererBase;
}

namespace RenderResources
{
    class IndexBuffer
    {
    public:
        IndexBuffer(const eastl::string& name);

        bool Create(uint32_t stride, uint32_t indexCount, RHI::ERHIMemoryType memoryType);

        RHI::RHIBuffer* GetBuffer() const { return m_pBuffer.get(); }
        uint32_t GetIndexCount() const { return m_IndexCount; }
        RHI::ERHIFormat GetFormat() const { return m_pBuffer->GetDesc().Format; }
    
    protected:
        eastl::string m_Name;

        eastl::unique_ptr<RHI::RHIBuffer> m_pBuffer;
        uint32_t m_IndexCount = 0;
    };
}