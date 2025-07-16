#include "TypedBuffer.hpp"
#include "Core/VultanaEngine.hpp"
#include "Renderer/RendererBase.hpp"

namespace RenderResources
{
    bool TypedBuffer::Create(RHI::ERHIFormat format, uint32_t elementCount, RHI::ERHIMemoryType memType, bool isUAV)
    {
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        RHI::RHIDevice* pDevice = pRenderer->GetDevice();

        uint32_t stride = RHI::GetFormatRowPitch(format, 1);

        RHI::RHIBufferDesc desc;
        desc.Stride = stride;
        desc.Size = stride * elementCount;
        desc.Format = format;
        desc.MemoryType = memType;
        desc.Usage = RHI::RHIBufferUsageTypedBuffer;

        if (isUAV)
        {
            desc.Usage |= RHI::RHIBufferUsageUnorderedAccess;
        }

        m_pBuffer.reset(pDevice->CreateBuffer(desc, m_Name));
        if (m_pBuffer == nullptr) return false;

        RHI::RHIShaderResourceViewDesc srvDesc;
        srvDesc.Format = format;
        srvDesc.Type = RHI::ERHIShaderResourceViewType::TypedBuffer;
        srvDesc.Buffer.Size = stride * elementCount;
        srvDesc.Buffer.Offset = 0;
        m_pSRV.reset(pDevice->CreateShaderResourceView(m_pBuffer.get(), srvDesc, m_Name));
        if (m_pSRV == nullptr) return false;

        if (isUAV)
        {
            RHI::RHIUnorderedAccessViewDesc uavDesc;
            uavDesc.Format = format;
            uavDesc.Type = RHI::ERHIUnorderedAccessViewType::TypedBuffer;
            uavDesc.Buffer.Size = stride * elementCount;
            uavDesc.Buffer.Offset = 0;
            m_pUAV.reset(pDevice->CreateUnorderedAccessView(m_pBuffer.get(), uavDesc, m_Name));
            if (m_pUAV == nullptr) return false;
        }

        return true;
    }
}