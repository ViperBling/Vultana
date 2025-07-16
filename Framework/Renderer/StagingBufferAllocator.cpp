#include "StagingBufferAllocator.hpp"
#include "RendererBase.hpp"

#include "Utilities/Math.hpp"

namespace Renderer
{
    StagingBufferAllocator::StagingBufferAllocator(RendererBase *renderer)
    {
        m_pRenderer = renderer;
    }

    StagingBuffer StagingBufferAllocator::Allocate(uint32_t size)
    {
        assert(size <= RHI::RHI_MAX_BUFFER_SIZE);

        if (m_Buffers.empty())
        {
            CreateNewBuffer();
        }

        if (m_AllocatedSize + size > RHI::RHI_MAX_BUFFER_SIZE)
        {
            CreateNewBuffer();
            m_CurrentBuffer++;
            m_AllocatedSize = 0;
        }

        StagingBuffer buffer;
        buffer.Buffer = m_Buffers[m_CurrentBuffer].get();
        buffer.Size = size;
        buffer.Offset = m_AllocatedSize;

        m_AllocatedSize += RoundUpPow2(size, 512);
        m_LastAllocatedFrame = m_pRenderer->GetFrameID();

        return buffer;
    }

    void StagingBufferAllocator::Reset()
    {
        m_CurrentBuffer = 0;
        m_AllocatedSize = 0;

        if (!m_Buffers.empty())
        {
            // 超时销毁
            if (m_pRenderer->GetFrameID() - m_LastAllocatedFrame > 100)
            {
                m_Buffers.clear();
            }
        }
    }

    void StagingBufferAllocator::CreateNewBuffer()
    {
        RHI::RHIBufferDesc desc;
        desc.Size = RHI::RHI_MAX_BUFFER_SIZE;
        desc.MemoryType = RHI::ERHIMemoryType::CPUOnly;

        RHI::RHIBuffer* buffer = m_pRenderer->GetDevice()->CreateBuffer(desc, "StagingBufferAllocator:m_pBuffer");
        m_Buffers.push_back(eastl::unique_ptr<RHI::RHIBuffer>(buffer));
    }
}