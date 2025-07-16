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

        Scene::World* GetWorld() const { return m_pWorld.get(); }
        Renderer::RendererBase* GetRenderer() const { return m_pRenderer.get(); }
        Editor::VultanaEditor* GetEditor() const { return m_pEditor.get(); }
        enki::TaskScheduler* GetTaskScheduler() const { return m_pTaskScheduler.get(); }

        void* GetWindowHandle() const { return m_WndHandle; }
        float GetDeltaTime() const { return m_FrameTime; }

        const eastl::string& GetWorkingPath() const { return m_WorkingPath; }
        const eastl::string& GetAssetsPath() const { return m_AssetsPath; }
        const eastl::string& GetShaderPath() const { return m_ShaderPath; }

    private:
        ~VultanaEngine();

    public:
        sigslot::signal<void*, uint32_t, uint32_t> OnWindowResizeSignal;

    private:
        eastl::unique_ptr<Scene::World> m_pWorld;
        eastl::unique_ptr<Renderer::RendererBase> m_pRenderer;
        eastl::unique_ptr<Editor::VultanaEditor> m_pEditor;

        eastl::unique_ptr<class enki::TaskScheduler> m_pTaskScheduler;

        void* m_WndHandle = nullptr;

        uint64_t m_LastFrameTime = 0;
        float m_FrameTime = 0.0f;

        eastl::string m_WorkingPath;
        eastl::string m_AssetsPath;
        eastl::string m_ShaderPath;
    };
} // namespace Vultana

