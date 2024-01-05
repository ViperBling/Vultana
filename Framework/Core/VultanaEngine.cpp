#include "VultanaEngine.hpp"

#define SOKOL_IMPL
#include <sokol/sokol_time.h>

namespace Core
{
    Engine *Engine::GetEngineInstance()
    {
        static Engine engine;
        return &engine;
    }

    void Engine::Init(Window::GLFWindow* windowHandle, uint32_t width, uint32_t height)
    {
        mpWorld = std::make_unique<Scene::World>();

        mWndHandle = windowHandle;

        Renderer::RendererCreateInfo rendererCI;
        rendererCI.ApplicationName = "Vultana";
        // rendererCI.DeviceType = RHI::RHIDeviceType::Hardware;
        rendererCI.Width = width;
        rendererCI.Height = height;
        rendererCI.bEnableValidationLayers = true;

        mpRenderer = std::make_unique<Renderer::RendererBase>(mWndHandle);
        mpRenderer->Init(rendererCI);

        stm_setup();
    }

    void Engine::Shutdown()
    {
        mpWorld.reset();
        mpRenderer.reset();
    }

    void Engine::Tick()
    {
        mFrameTime = (float)stm_sec(stm_laptime(&mLastFrameTime));

        mpWorld->Tick(mFrameTime);
        mpRenderer->RenderFrame();
    }
}