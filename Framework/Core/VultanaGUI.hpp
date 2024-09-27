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
    
    private:
        // TODO
    };
}