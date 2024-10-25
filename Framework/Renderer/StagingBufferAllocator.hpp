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
        RendererBase* mpRenderer = nullptr;
        eastl::vector<eastl::unique_ptr<RHI::RHIBuffer>> mBuffers;
        uint32_t mCurrentBuffer = 0;
        uint32_t mAllocatedSize = 0;
        uint64_t mLastAllocatedFrame = 0;
    };
}