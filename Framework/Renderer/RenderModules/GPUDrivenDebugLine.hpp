#pragma once

#include "Renderer/RenderResources/RawBuffer.hpp"
#include "Renderer/RenderResources/StructuredBuffer.hpp"

namespace Renderer
{
    class GPUDrivenDebugLine
    {
    public:
        GPUDrivenDebugLine(Renderer::RendererBase* pRenderer);
        
        void Clear(RHI::RHICommandList* pCmdList);
        void PrepareForDraw(RHI::RHICommandList* pCmdList);
        void Draw(RHI::RHICommandList* pCmdList);

        RHI::RHIDescriptor* GetVertexBufferSRV() const { return m_pLineVertexBuffer->GetSRV(); }
        RHI::RHIDescriptor* GetVertexBufferUAV() const { return m_pLineVertexBuffer->GetUAV(); }
        RHI::RHIDescriptor* GetDrawArgsBufferUAV() const { return m_pDrawArgsBuffer->GetUAV(); }

    private:
        Renderer::RendererBase* m_pRenderer = nullptr;
        RHI::RHIPipelineState* m_pPSO = nullptr;

        eastl::unique_ptr<RenderResources::RawBuffer> m_pDrawArgsBuffer = nullptr;
        eastl::unique_ptr<RenderResources::StructuredBuffer> m_pLineVertexBuffer = nullptr;
    };
}