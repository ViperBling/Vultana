#include "IndexBuffer.hpp"
#include "Core/VultanaEngine.hpp"
#include "Renderer/RendererBase.hpp"

#include <cassert>

namespace RenderResources
{
    FIndexBuffer::FIndexBuffer(const eastl::string &name)
    {
        m_Name = name;
    }

    bool FIndexBuffer::Create(uint32_t stride, uint32_t indexCount, RHI::ERHIMemoryType memoryType)
    {
        assert(stride == 2 || stride == 4);
        m_IndexCount = indexCount;

        Renderer::FRendererBase *renderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
        RHI::FRHIDevice *device = renderer->GetDevice();

        RHI::FRHIBufferDesc desc;
        desc.Stride = stride;
        desc.Size = stride * indexCount;
        desc.Format = stride == 2 ? RHI::ERHIFormat::R16UI : RHI::ERHIFormat::R32UI;
        desc.MemoryType = memoryType;

        m_pBuffer.reset(device->CreateBuffer(desc, m_Name));
        if (m_pBuffer == nullptr)
        {
            return false;
        }
        return true;
    }
}