#include "VultanaEngine.hpp"
#include "VultanaGUI.hpp"
#include "VultanaEditor.hpp"
#include "Utilities/Log.hpp"

#include <rpmalloc/rpmalloc.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#define SOKOL_IMPL
#include <sokol/sokol_time.h>
#include <ImGui/imgui.h>
#include <SimpleIni.h>

namespace Core
{
    VultanaEngine *VultanaEngine::GetEngineInstance()
    {
        static VultanaEngine engine;
        return &engine;
    }

    void VultanaEngine::Init(void* windowHandle, uint32_t width, uint32_t height)
    {
        mWorkingPath = "../";
        // mAssetsPath = "../Assets/";
        // mShaderPath = "../Shaders/";

        auto console_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("RealEngine", spdlog::sinks_init_list{ console_sink});

        spdlog::set_default_logger(logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [thread %t] %v");
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_every(std::chrono::milliseconds(10));

        mWndHandle = windowHandle;

        eastl::string configFile = mWorkingPath + "Config/VultanaEngine.ini";
        CSimpleIniA configIni;
        if (configIni.LoadFile(configFile.c_str()) < 0)
        {
            VTNA_LOG_ERROR("Failed to load config file: {}", configFile);
        }

        mAssetsPath = configIni.GetValue("Vultana", "AssetsPath");
        mShaderPath = configIni.GetValue("Vultana", "ShaderPath");

        RHI::ERHIRenderBackend renderBackend = RHI::ERHIRenderBackend::Vulkan;

        mpRenderer = eastl::make_unique<Renderer::RendererBase>();
        if (!mpRenderer->CreateDevice(renderBackend, mWndHandle, width, height))
        {
            VTNA_LOG_ERROR("Failed to create renderer device");
            exit(0);
        }

        mpWorld = eastl::make_unique<Scene::World>();
        mpWorld->LoadScene(mAssetsPath + configIni.GetValue("World", "SceneFile"));

        mpGUI = eastl::make_unique<GUI>();
        mpGUI->Init();

        mpEditor = eastl::make_unique<VultanaEditor>();

        stm_setup();
    }

    void VultanaEngine::Shutdown()
    {
        mpWorld.reset();
        mpGUI.reset();
        mpEditor.reset();
        mpRenderer.reset();

        spdlog::shutdown();
    }

    void VultanaEngine::Tick()
    {
        mFrameTime = (float)stm_sec(stm_laptime(&mLastFrameTime));

        mpGUI->Tick();

        ImGuiIO& io = ImGui::GetIO();
        bool isMinimized = io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f;

        if (isMinimized)
        {
            ImGui::Render();
        }
        else
        {
            mpEditor->Tick();
            mpWorld->Tick(mFrameTime);
            mpRenderer->RenderFrame();
        }

        
    }
    
    VultanaEngine::~VultanaEngine()
    {
        rpmalloc_finalize();
    }
}