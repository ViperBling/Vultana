#pragma once

#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/functional.h>

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

        void AddCommand(const eastl::function<void()>& cmd) { mCommands.push_back(cmd); }
    
    private:
        void SetupRenderStates(RHI::RHICommandList* pCmdList, uint32_t frameIdx);
    
    private:
        RHI::RHIPipelineState* mpPSO;

        eastl::unique_ptr<RenderResources::Texture2D> mpFontTexture;
        eastl::unique_ptr<RenderResources::StructuredBuffer> mpVertexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];
        eastl::unique_ptr<RenderResources::IndexBuffer> mpIndexBuffer[RHI::RHI_MAX_INFLIGHT_FRAMES];

        eastl::vector<eastl::function<void()>> mCommands;
    };
}