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

        void Init(void* windowHandle, uint32_t width, uint32_t height);
        void Shutdown();
        void Tick();

        Scene::World* GetWorld() const { return mpWorld.get(); }
        Renderer::RendererBase* GetRenderer() const { return mpRenderer.get(); }

        void* GetWindowHandle() const { return mWndHandle; }
        float GetDeltaTime() const { return mFrameTime; }

    private:
        std::unique_ptr<Scene::World> mpWorld;
        std::unique_ptr<Renderer::RendererBase> mpRenderer;

        void* mWndHandle;
        uint64_t mLastFrameTime = 0;
        float mFrameTime = 0.0f;
    };
} // namespace Vultana

