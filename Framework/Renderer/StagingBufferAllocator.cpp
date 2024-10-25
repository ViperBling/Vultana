#include "StagingBufferAllocator.hpp"
#include "RendererBase.hpp"

#include "Utilities/Math.hpp"

namespace Renderer
{
    StagingBufferAllocator::StagingBufferAllocator(RendererBase *renderer)
    {
        mpRenderer = renderer;
    }

    StagingBuffer StagingBufferAllocator::Allocate(uint32_t size)
    {
        assert(size <= RHI::RHI_MAX_BUFFER_SIZE);

        if (mBuffers.empty())
        {
            CreateNewBuffer();
        }

        if (mAllocatedSize + size > RHI::RHI_MAX_BUFFER_SIZE)
        {
            CreateNewBuffer();
            mCurrentBuffer++;
            mAllocatedSize = 0;
        }

        StagingBuffer buffer;
        buffer.Buffer = mBuffers[mCurrentBuffer].get();
        buffer.Size = size;
        buffer.Offset = mAllocatedSize;

        mAllocatedSize += RoundUpPow2(size, 512);
        mLastAllocatedFrame = mpRenderer->GetFrameID();

        return buffer;
    }

    void StagingBufferAllocator::Reset()
    {
        mCurrentBuffer = 0;
        mAllocatedSize = 0;

        if (!mBuffers.empty())
        {
            // 超时销毁
            if (mpRenderer->GetFrameID() - mLastAllocatedFrame > 100)
            {
                mBuffers.clear();
            }
        }
    }

    void StagingBufferAllocator::CreateNewBuffer()
    {
        RHI::RHIBufferDesc desc;
        desc.Size = RHI::RHI_MAX_BUFFER_SIZE;
        desc.MemoryType = RHI::ERHIMemoryType::CPUOnly;

        RHI::RHIBuffer* buffer = mpRenderer->GetDevice()->CreateBuffer(desc, "StagingBufferAllocator:mpBuffer");
        mBuffers.push_back(eastl::unique_ptr<RHI::RHIBuffer>(buffer));
    }
}