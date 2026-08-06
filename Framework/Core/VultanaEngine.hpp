#pragma once

#include "Scene/World.hpp"
#include "Editor/VultanaEditor.hpp"
#include "Renderer/RendererBase.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/EntityFactory.hpp"
#include "Scene/ECS/AssetRegistry.hpp"
#include "Scene/ECS/RenderSink.hpp"
#include "Renderer/ECSRenderBridge.hpp"

#include <sigslot/signal.hpp>
#include <iostream>

namespace enki 
{ 
    class TaskScheduler; 
}

namespace Editor
{
    class FVultanaEditor;
}

namespace Core
{
    class FVultanaEngine
    {
    public:
        static FVultanaEngine* GetEngineInstance();

        void Init(void* windowHandle, uint32_t width, uint32_t height);
        void Shutdown();
        void Tick();

        Scene::FWorld* GetWorld() const { return m_pWorld.get(); }
        Renderer::FRendererBase* GetRenderer() const { return m_pRenderer.get(); }
        Editor::FVultanaEditor* GetEditor() const { return m_pEditor.get(); }
        enki::TaskScheduler* GetTaskScheduler() const { return m_pTaskScheduler.get(); }
        Scene::ECS::FEntitySubsystem *GetEntitySystem() const { return m_pEntitySystem.get(); }
        Scene::ECS::IAssetRegistry* GetAssetRegistry() const { return m_pAssetRegistry.get(); }
        Scene::ECS::ISceneRenderSink* GetRenderSink() const { return m_pRenderSink.get(); }
        Scene::ECS::FEntityFactory* GetEntityFactory() const { return m_pEntityFactory.get(); }
        bool IsUseECSBackend() const { return m_bUseECSBackend; }

        void* GetWindowHandle() const { return m_WndHandle; }
        float GetDeltaTime() const { return m_FrameTime; }

        const eastl::string& GetWorkingPath() const { return m_WorkingPath; }
        const eastl::string& GetAssetsPath() const { return m_AssetsPath; }
        const eastl::string& GetShaderPath() const { return m_ShaderPath; }

    private:
        ~FVultanaEngine();

    public:
        sigslot::signal<void*, uint32_t, uint32_t> OnWindowResizeSignal;

    private:
        eastl::unique_ptr<Scene::FWorld> m_pWorld;
        eastl::unique_ptr<Renderer::FRendererBase> m_pRenderer;
        eastl::unique_ptr<Editor::FVultanaEditor> m_pEditor;

        eastl::unique_ptr<class enki::TaskScheduler> m_pTaskScheduler;

        void* m_WndHandle = nullptr;

        uint64_t m_LastFrameTime = 0;
        float m_FrameTime = 0.0f;

        eastl::string m_WorkingPath;
        eastl::string m_AssetsPath;
        eastl::string m_ShaderPath;

        eastl::unique_ptr<Scene::ECS::FEntitySubsystem> m_pEntitySystem;
        eastl::unique_ptr<Scene::ECS::IAssetRegistry> m_pAssetRegistry;
        eastl::unique_ptr<Scene::ECS::ISceneRenderSink> m_pRenderSink;
        eastl::unique_ptr<Scene::ECS::FEntityFactory> m_pEntityFactory;
        bool m_bUseECSBackend = false;
    };
} // namespace Vultana

