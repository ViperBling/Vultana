#include "RawBuffer.hpp"
#include "Core/VultanaEngine.hpp"
#include "Renderer/RendererBase.hpp"

namespace RenderResources
{
    RawBuffer::RawBuffer(const std::string &name)
    {
        mName = name;
    }

    bool RawBuffer::Create(uint32_t size, RHI::ERHIMemoryType memoryType, bool isUAV)
    {
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto pDevice = pRenderer->GetDevice();

        assert(size % 4 == 0);

        RHI::RHIBufferDesc desc;
        desc.Stride = 4;
        desc.Size = size;
        desc.Format = RHI::ERHIFormat::R32F;
        desc.MemoryType = memoryType;
        desc.Usage = RHI::RHIBufferUsageRawBuffer;

        if (isUAV)
        {
            desc.Usage |= RHI::RHIBufferUsageUnorderedAccess;
        }
        mpBuffer.reset(pDevice->CreateBuffer(desc, mName + "_RawBuffer"));
        if (mpBuffer == nullptr)
        {
            return false;
        }

        RHI::RHIShaderResourceViewDesc srvDesc;
        srvDesc.Type = RHI::ERHIShaderResourceViewType::RawBuffer;
        srvDesc.Buffer.Offset = 0;
        srvDesc.Buffer.Size = size;
        mpSRV.reset(pDevice->CreateShaderResourceView(mpBuffer.get(), srvDesc, mName + "_SRV"));
        if (mpSRV == nullptr)
        {
            return false;
        }

        if (isUAV)
        {
            RHI::RHIUnorderedAccessViewDesc uavDesc;
            uavDesc.Type = RHI::ERHIUnorderedAccessViewType::RawBuffer;
            uavDesc.Buffer.Offset = 0;
            uavDesc.Buffer.Size = size;
            mpUAV.reset(pDevice->CreateUnorderedAccessView(mpBuffer.get(), uavDesc, mName + "_UAV"));
            if (mpUAV == nullptr)
            {
                return false;
            }
        }
        return true;
    }
}