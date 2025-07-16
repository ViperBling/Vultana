#pragma once

#include "RHI/RHI.hpp"

#include <EASTL/unique_ptr.h>

namespace Renderer
{
    class RendererBase;

    struct StagingBuffer
    {
        RHI::RHIBuffer* Buffer;
        uint32_t Offset;
        uint32_t Size;
    };

    class StagingBufferAllocator
    {
    public:
        StagingBufferAllocator(RendererBase* renderer);
        
        StagingBuffer Allocate(uint32_t size);
        void Reset();

    private:
        void CreateNewBuffer();

    private:
        RendererBase* m_pRenderer = nullptr;
        eastl::vector<eastl::unique_ptr<RHI::RHIBuffer>> m_Buffers;
        uint32_t m_CurrentBuffer = 0;
        uint32_t m_AllocatedSize = 0;
        uint64_t m_LastAllocatedFrame = 0;
    };
}