#pragma once

#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/functional.h>

namespace Editor
{
    class FImGuiImplement
    {
    public:
        FImGuiImplement(Renderer::FRendererBase* pRenderer);
        ~FImGuiImplement();

        bool Init();
        void NewFrame();
        void Render(RHI::FRHICommandList* pCmdList);
    
    private:
        void SetupRenderStates(RHI::FRHICommandList* pCmdList, uint32_t frameIdx);
    
    private:
        Renderer::FRendererBase* m_pRenderer = nullptr;
        RHI::FRHIPipelineState* m_pPSO;

        eastl::unique_ptr<RenderResources::FTexture2D> m_pFontTexture;
        eastl::unique_ptr<RenderResources::FStructuredBuffer> m_pVertexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];
        eastl::unique_ptr<RenderResources::FIndexBuffer> m_pIndexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];
    };
}