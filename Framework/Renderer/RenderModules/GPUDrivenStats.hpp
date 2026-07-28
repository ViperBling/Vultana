#pragma once 

#include "RHI/RHI.hpp"
#include "Renderer/RenderResources/TypedBuffer.hpp"

namespace Renderer
{
    class RendererBase;

    class GPUDrivenStats
    {
    public:
        GPUDrivenStats(RendererBase* pRenderer);

        void Clear(RHI::RHICommandList *pCmdList);
        void Readback(RHI::RHICommandList *pCmdList);
        void OnGui();

        RHI::RHIDescriptor* GetStatsBufferUAV() const { return m_pStatsBuffer->GetUAV(); }
        uint32_t GetCounterValue(uint32_t index) const;

    private:
        RendererBase* m_pRenderer = nullptr;

        eastl::unique_ptr<RenderResources::TypedBuffer> m_pStatsBuffer;
        eastl::unique_ptr<RHI::RHIBuffer> m_pReadbackBuffers[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint32_t m_ReadbackValues[16] = {};
    };
}