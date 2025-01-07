#pragma once

#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/functional.h>

namespace Core
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
        Renderer::RendererBase* mpRenderer = nullptr;
        RHI::RHIPipelineState* mpPSO;

        eastl::unique_ptr<RenderResources::Texture2D> mpFontTexture;
        eastl::unique_ptr<RenderResources::StructuredBuffer> mpVertexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];
        eastl::unique_ptr<RenderResources::IndexBuffer> mpIndexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];
    };
}