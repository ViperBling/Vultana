#pragma once

#include "Scene/World.hpp"
#include "Editor/VultanaEditor.hpp"
#include "Renderer/RendererBase.hpp"

#include <sigslot/signal.hpp>
#include <iostream>

namespace enki 
{ 
    class TaskScheduler; 
}

namespace Editor
{
    class VultanaEditor;
}

namespace Core
{
    class VultanaEngine
    {
    public:
        static VultanaEngine* GetEngineInstance();

        void Init(void* windowHandle, uint32_t width, uint32_t height);
        void Shutdown();
        void Tick();

        Scene::World* GetWorld() const { return mpWorld.get(); }
        Renderer::RendererBase* GetRenderer() const { return mpRenderer.get(); }
        Editor::VultanaEditor* GetEditor() const { return mpEditor.get(); }
        enki::TaskScheduler* GetTaskScheduler() const { return mpTaskScheduler.get(); }

        void* GetWindowHandle() const { return mWndHandle; }
        float GetDeltaTime() const { return mFrameTime; }

        const eastl::string& GetWorkingPath() const { return mWorkingPath; }
        const eastl::string& GetAssetsPath() const { return mAssetsPath; }
        const eastl::string& GetShaderPath() const { return mShaderPath; }

    private:
        ~VultanaEngine();

    public:
        sigslot::signal<void*, uint32_t, uint32_t> OnWindowResizeSignal;

    private:
        eastl::unique_ptr<Scene::World> mpWorld;
        eastl::unique_ptr<Renderer::RendererBase> mpRenderer;
        eastl::unique_ptr<Editor::VultanaEditor> mpEditor;

        eastl::unique_ptr<class enki::TaskScheduler> mpTaskScheduler;

        void* mWndHandle = nullptr;

        uint64_t mLastFrameTime = 0;
        float mFrameTime = 0.0f;

        eastl::string mWorkingPath;
        eastl::string mAssetsPath;
        eastl::string mShaderPath;
    };
} // namespace Vultana

