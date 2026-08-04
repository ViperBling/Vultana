#pragma once 

#include "RHI/RHI.hpp"
#include "Renderer/RenderResources/TypedBuffer.hpp"

namespace Renderer
{
    class FRendererBase;

    class FGPUDrivenStats
    {
    public:
        FGPUDrivenStats(FRendererBase* pRenderer);

        void Clear(RHI::FRHICommandList *pCmdList);
        void Readback(RHI::FRHICommandList *pCmdList);
        void OnGui();

        RHI::FRHIDescriptor* GetStatsBufferUAV() const { return m_pStatsBuffer->GetUAV(); }
        uint32_t GetCounterValue(uint32_t index) const;

    private:
        FRendererBase* m_pRenderer = nullptr;

        eastl::unique_ptr<RenderResources::FTypedBuffer> m_pStatsBuffer;
        eastl::unique_ptr<RHI::FRHIBuffer> m_pReadbackBuffers[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint32_t m_ReadbackValues[16] = {};
    };
}