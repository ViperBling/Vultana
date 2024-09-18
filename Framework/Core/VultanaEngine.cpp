#include "VultanaEngine.hpp"
#include "Utilities/Log.hpp"

#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#define SOKOL_IMPL
#include <sokol/sokol_time.h>

namespace Core
{
    VultanaEngine *VultanaEngine::GetEngineInstance()
    {
        static VultanaEngine engine;
        return &engine;
    }

    void VultanaEngine::Init(Window::GLFWindow* windowHandle, uint32_t width, uint32_t height)
    {
        mAssetsPath = "Assets/";
        mShaderPath = "Shaders/";

        auto console_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("RealEngine", spdlog::sinks_init_list{ console_sink});

        spdlog::set_default_logger(logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [thread %t] %v");
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_every(std::chrono::milliseconds(10));

        mpWorld = std::make_unique<Scene::World>();

        mWndHandle = windowHandle;

        RHI::ERHIRenderBackend renderBackend = RHI::ERHIRenderBackend::Vulkan;

        mpRenderer = std::make_unique<Renderer::RendererBase>();
        if (!mpRenderer->CreateDevice(renderBackend, mWndHandle, width, height))
        {
            VTNA_LOG_ERROR("Failed to create renderer device");
            exit(0);
        }

        stm_setup();
    }

    void VultanaEngine::Shutdown()
    {
        mpWorld.reset();
        mpRenderer.reset();

        spdlog::shutdown();
    }

    void VultanaEngine::Tick()
    {
        mFrameTime = (float)stm_sec(stm_laptime(&mLastFrameTime));

        mpWorld->Tick(mFrameTime);
        mpRenderer->RenderFrame();
    }
    
    VultanaEngine::~VultanaEngine()
    {
    }
}