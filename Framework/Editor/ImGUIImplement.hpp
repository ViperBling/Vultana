#pragma once

#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/functional.h>

namespace Editor
{
    class ImGuiImplement
    {
    public:
        ImGuiImplement(Renderer::RendererBase* pRenderer);
        ~ImGuiImplement();

        bool Init();
        void NewFrame();
        void Render(RHI::RHICommandList* pCmdList);
    
    private:
        void SetupRenderStates(RHI::RHICommandList* pCmdList, uint32_t frameIdx);
    
    private:
        Renderer::RendererBase* m_pRenderer = nullptr;
        RHI::RHIPipelineState* m_pPSO;

        eastl::unique_ptr<RenderResources::Texture2D> m_pFontTexture;
        eastl::unique_ptr<RenderResources::StructuredBuffer> m_pVertexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];
        eastl::unique_ptr<RenderResources::IndexBuffer> m_pIndexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];
    };
}