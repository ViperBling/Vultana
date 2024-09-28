#pragma once

#include "Renderer/RendererBase.hpp"
#include <functional>

namespace Core
{
    class GUI
    {
    public:
        GUI();
        ~GUI();

        bool Init();
        void Tick();
        void Render(RHI::RHICommandList* pCmdList);

        void AddCommand(const std::function<void()>& cmd) { mCommands.push_back(cmd); }
    
    private:
        void SetupRenderStates(RHI::RHICommandList* pCmdList, uint32_t frameIdx);
    
    private:
        RHI::RHIPipelineState* mpPSO;

        std::unique_ptr<RenderResources::Texture2D> mpFontTexture;
        std::unique_ptr<RenderResources::StructuredBuffer> mpVertexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];
        std::unique_ptr<RenderResources::IndexBuffer> mpIndexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];

        std::vector<std::function<void()>> mCommands;
    };
}