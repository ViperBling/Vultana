#pragma once

#include "Renderer/RenderResources/RawBuffer.hpp"
#include "Renderer/RenderResources/StructuredBuffer.hpp"

namespace Renderer
{
    class FGPUDrivenDebugLine
    {
    public:
        FGPUDrivenDebugLine(Renderer::FRendererBase* pRenderer);
        
        void Clear(RHI::FRHICommandList* pCmdList);
        void PrepareForDraw(RHI::FRHICommandList* pCmdList);
        void Draw(RHI::FRHICommandList* pCmdList);

        RHI::FRHIDescriptor* GetVertexBufferSRV() const { return m_pLineVertexBuffer->GetSRV(); }
        RHI::FRHIDescriptor* GetVertexBufferUAV() const { return m_pLineVertexBuffer->GetUAV(); }
        RHI::FRHIDescriptor* GetDrawArgsBufferUAV() const { return m_pDrawArgsBuffer->GetUAV(); }

    private:
        Renderer::FRendererBase* m_pRenderer = nullptr;
        RHI::FRHIPipelineState* m_pPSO = nullptr;

        eastl::unique_ptr<RenderResources::FRawBuffer> m_pDrawArgsBuffer = nullptr;
        eastl::unique_ptr<RenderResources::FStructuredBuffer> m_pLineVertexBuffer = nullptr;
    };
}