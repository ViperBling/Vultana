#include "VultanaEngine.hpp"

#define SOKOL_IMPL
#include <sokol/sokol_time.h>

namespace Vultana
{
    Engine *Engine::GetEngineInstance()
    {
        static Engine engine;
        return &engine;
    }

    void Engine::Init(GLFWindow* windowHandle, uint32_t width, uint32_t height)
    {
        mpWorld = std::make_unique<World>();

        mWndHandle = std::unique_ptr<GLFWindow>(windowHandle);

        RendererCreateInfo rendererCI;
        rendererCI.Extensions = mWndHandle->GetRequiredExtensions();
        rendererCI.ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
        rendererCI.ApplicationName = "Vultana";
        rendererCI.DeviceType = RHIDeviceType::DISCRETE_GPU;

        mpRenderer = std::make_unique<RendererBase>(rendererCI);
        mpRenderer->Init(mWndHandle->CreateWindowSurface(*mpRenderer), rendererCI);

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