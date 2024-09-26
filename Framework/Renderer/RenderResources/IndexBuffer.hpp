#pragma once

#include "RHI/RHI.hpp"

#include <memory>

namespace Renderer
{
    class RendererBase;
}

namespace RenderResources
{
    class IndexBuffer
    {
    public:
        IndexBuffer(const std::string& name);

        bool Create(uint32_t stride, uint32_t indexCount, RHI::ERHIMemoryType memoryType);

        RHI::RHIBuffer* GetBuffer() const { return mpBuffer.get(); }
        uint32_t GetIndexCount() const { return mIndexCount; }
        RHI::ERHIFormat GetFormat() const { return mpBuffer->GetDesc().Format; }
    
    protected:
        std::string mName;

        std::unique_ptr<RHI::RHIBuffer> mpBuffer;
        uint32_t mIndexCount = 0;
    };
}