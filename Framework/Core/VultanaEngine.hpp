#pragma once

#include <iostream>

#include "Scene/World.hpp"

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
        void* GetWindowHandle() const { return mWndHandle; }
        float GetDeltaTime() const { return mFrameTime; }

    private:
        std::unique_ptr<Scene::World> mpWorld;

        void* mWndHandle;
        uint64_t mLastFrameTime = 0;
        float mFrameTime = 0.0f;
    };
} // namespace Vultana

