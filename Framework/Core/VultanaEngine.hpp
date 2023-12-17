#pragma once

#include "Scene/World.hpp"
#include "Renderer/Renderer.hpp"
#include "Windows/GLFWindow.hpp"

#include <iostream>

namespace Vultana
{
    class Engine
    {
    public:
        static Engine* GetEngineInstance();

        void Init(GLFWindow* windowHandle, uint32_t width, uint32_t height);
        void Shutdown();
        void Tick();

        World* GetWorld() const { return mpWorld.get(); }
        RendererBase* GetRenderer() const { return mpRenderer.get(); }

        GLFWindow* GetWindowHandle() const { return mWndHandle; }
        float GetDeltaTime() const { return mFrameTime; }

    private:
        std::unique_ptr<World> mpWorld;
        std::unique_ptr<RendererBase> mpRenderer;

        GLFWindow* mWndHandle;

        uint64_t mLastFrameTime = 0;
        float mFrameTime = 0.0f;
    };
} // namespace Vultana

