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

        RHI::RHIDescriptor* GetVertexBufferSRV() const { return mpLineVertexBuffer->GetSRV(); }
        RHI::RHIDescriptor* GetVertexBufferUAV() const { return mpLineVertexBuffer->GetUAV(); }
        RHI::RHIDescriptor* GetDrawArgsBufferUAV() const { return mpDrawArgsBuffer->GetUAV(); }

    private:
        Renderer::RendererBase* mpRenderer = nullptr;
        RHI::RHIPipelineState* mpPSO = nullptr;

        eastl::unique_ptr<RenderResources::RawBuffer> mpDrawArgsBuffer = nullptr;
        eastl::unique_ptr<RenderResources::StructuredBuffer> mpLineVertexBuffer = nullptr;
    };
}