#include "StructuredBuffer.hpp"
#include "Core/VultanaEngine.hpp"
#include "Renderer/RendererBase.hpp"

namespace Renderer
{
    StructuredBuffer::StructuredBuffer(const std::string &name)
    {
        mName = name;
    }

    bool StructuredBuffer::Create(uint32_t stride, uint32_t elementCount, RHI::ERHIMemoryType memoryType, bool isUAV)
    {
        RendererBase *renderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        RHI::RHIDevice *device = renderer->GetDevice();

        RHI::RHIBufferDesc desc;
        desc.Stride = stride;
        desc.Size = stride * elementCount;
        desc.Format = RHI::ERHIFormat::Unknown;
        desc.MemoryType = memoryType;
        desc.Usage = RHI::RHIBufferUsageStructuredBuffer;

        if (isUAV)
        {
            desc.Usage |= RHI::RHIBufferUsageUnorderedAccess;
        }

        mpBuffer.reset(device->CreateBuffer(desc, mName));
        if (mpBuffer == nullptr)
        {
            return false;
        }

        RHI::RHIShaderResourceViewDesc srvDesc;
        srvDesc.Type = RHI::ERHIShaderResourceViewType::StructuredBuffer;
        srvDesc.Buffer.Size = elementCount * stride;
        mpSRV.reset(device->CreateShaderResourceView(mpBuffer.get(), srvDesc, mName + "_SRV"));
        if (mpSRV == nullptr)
        {
            return false;
        }

        if (isUAV)
        {
            RHI::RHIUnorderedAccessViewDesc uavDesc;
            uavDesc.Type = RHI::ERHIUnorderedAccessViewType::StructuredBuffer;
            uavDesc.Buffer.Size = elementCount * stride;
            mpUAV.reset(device->CreateUnorderedAccessView(mpBuffer.get(), uavDesc, mName + "_UAV"));
            if (mpUAV == nullptr)
            {
                return false;
            }
        }
        return true;
    }
}