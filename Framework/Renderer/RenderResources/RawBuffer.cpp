#include "RawBuffer.hpp"
#include "Core/VultanaEngine.hpp"
#include "Renderer/RendererBase.hpp"

namespace RenderResources
{
    FRawBuffer::FRawBuffer(const eastl::string &name)
    {
        m_Name = name;
    }

    bool FRawBuffer::Create(uint32_t size, RHI::ERHIMemoryType memoryType, bool isUAV)
    {
        auto pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
        auto pDevice = pRenderer->GetDevice();

        assert(size % 4 == 0);

        RHI::FRHIBufferDesc desc;
        desc.Stride = 4;
        desc.Size = size;
        desc.Format = RHI::ERHIFormat::R32F;
        desc.MemoryType = memoryType;
        desc.Usage = RHI::RHIBufferUsageRawBuffer;

        if (isUAV)
        {
            desc.Usage |= RHI::RHIBufferUsageUnorderedAccess;
        }
        m_pBuffer.reset(pDevice->CreateBuffer(desc, m_Name + "_RawBuffer"));
        if (m_pBuffer == nullptr)
        {
            return false;
        }

        RHI::FRHIShaderResourceViewDesc srvDesc;
        srvDesc.Type = RHI::ERHIShaderResourceViewType::RawBuffer;
        srvDesc.Buffer.Offset = 0;
        srvDesc.Buffer.Size = size;
        m_pSRV.reset(pDevice->CreateShaderResourceView(m_pBuffer.get(), srvDesc, m_Name + "_SRV"));
        if (m_pSRV == nullptr)
        {
            return false;
        }

        if (isUAV)
        {
            RHI::FRHIUnorderedAccessViewDesc uavDesc;
            uavDesc.Type = RHI::ERHIUnorderedAccessViewType::RawBuffer;
            uavDesc.Buffer.Offset = 0;
            uavDesc.Buffer.Size = size;
            m_pUAV.reset(pDevice->CreateUnorderedAccessView(m_pBuffer.get(), uavDesc, m_Name + "_UAV"));
            if (m_pUAV == nullptr)
            {
                return false;
            }
        }
        return true;
    }
}