#pragma once

#include "Scene/World.hpp"
#include "Renderer/RendererBase.hpp"

#include <sigslot/signal.hpp>
#include <iostream>

namespace Core
{
    class GUI;
    class VultanaEditor;
    
    class VultanaEngine
    {
    public:
        static VultanaEngine* GetEngineInstance();

        void Init(void* windowHandle, uint32_t width, uint32_t height);
        void Shutdown();
        void Tick();

        Scene::World* GetWorld() const { return mpWorld.get(); }
        GUI* GetGUI() const { return mpGUI.get(); }
        Renderer::RendererBase* GetRenderer() const { return mpRenderer.get(); }
        VultanaEditor* GetEditor() const { return mpEditor.get(); }

        void* GetWindowHandle() const { return mWndHandle; }
        float GetDeltaTime() const { return mFrameTime; }

        const std::string& GetWorkingPath() const { return mWorkingPath; }
        const std::string& GetAssetsPath() const { return mAssetsPath; }
        const std::string& GetShaderPath() const { return mShaderPath; }

    private:
        ~VultanaEngine();

    public:
        sigslot::signal<void*, uint32_t, uint32_t> OnWindowResizeSignal;

    private:
        std::unique_ptr<Scene::World> mpWorld;
        std::unique_ptr<GUI> mpGUI;
        std::unique_ptr<Renderer::RendererBase> mpRenderer;
        std::unique_ptr<VultanaEditor> mpEditor;

        void* mWndHandle = nullptr;

        uint64_t mLastFrameTime = 0;
        float mFrameTime = 0.0f;

        std::string mWorkingPath;
        std::string mAssetsPath;
        std::string mShaderPath;
    };
} // namespace Vultana

