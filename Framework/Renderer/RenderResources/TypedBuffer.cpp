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

        mpBuffer.reset(pDevice->CreateBuffer(desc, mName));
        if (mpBuffer == nullptr) return false;

        RHI::RHIShaderResourceViewDesc srvDesc;
        srvDesc.Format = format;
        srvDesc.Type = RHI::ERHIShaderResourceViewType::TypedBuffer;
        srvDesc.Buffer.Size = stride * elementCount;
        srvDesc.Buffer.Offset = 0;
        mpSRV.reset(pDevice->CreateShaderResourceView(mpBuffer.get(), srvDesc, mName));
        if (mpSRV == nullptr) return false;

        if (isUAV)
        {
            RHI::RHIUnorderedAccessViewDesc uavDesc;
            uavDesc.Format = format;
            uavDesc.Type = RHI::ERHIUnorderedAccessViewType::TypedBuffer;
            uavDesc.Buffer.Size = stride * elementCount;
            uavDesc.Buffer.Offset = 0;
            mpUAV.reset(pDevice->CreateUnorderedAccessView(mpBuffer.get(), uavDesc, mName));
            if (mpUAV == nullptr) return false;
        }

        return true;
    }
}