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

        mpRenderer = std::make_unique<RendererBase>();
        mpRenderer->Init(mWndHandle.get(), width, height);

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