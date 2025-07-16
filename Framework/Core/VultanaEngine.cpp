#include "VultanaEngine.hpp"
#include "Editor/VultanaEditor.hpp"
#include "Utilities/Log.hpp"
#include "Utilities/String.hpp"

#include <rpmalloc/rpmalloc.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#define SOKOL_IMPL
#include <sokol/sokol_time.h>
#include <ImGui/imgui.h>
#include <SimpleIni.h>
#include <enkiTS/TaskScheduler.h>

namespace Core
{
    VultanaEngine *VultanaEngine::GetEngineInstance()
    {
        static VultanaEngine engine;
        return &engine;
    }

    void VultanaEngine::Init(void* windowHandle, uint32_t width, uint32_t height)
    {
        m_WorkingPath = "../";
        // m_AssetsPath = "../Assets/";
        // m_ShaderPath = "../Shaders/";

        auto console_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("RealEngine", spdlog::sinks_init_list{ console_sink});

        spdlog::set_default_logger(logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [thread %t] %v");
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_every(std::chrono::milliseconds(10));

        enki::TaskSchedulerConfig tsConfig;
        tsConfig.profilerCallbacks.threadStart = [](uint32_t i)
        {
            rpmalloc_thread_initialize();

            eastl::string threadName = fmt::format("WorkerThread {}", i).c_str();
            // Only in Windows
            SetThreadDescription(GetCurrentThread(), StringUtils::StringToWString(threadName).c_str());
        };
        tsConfig.profilerCallbacks.threadStop = [](uint32_t i)
        {
            rpmalloc_thread_finalize(1);
        };
        tsConfig.customAllocator.alloc = [](size_t align, size_t size, void* userData, const char* file, int line)
        {
            return VTNA_ALLOC(size, align);
        };
        tsConfig.customAllocator.free = [](void* ptr, size_t size, void* userData, const char* file, int line)
        {
            VTNA_FREE(ptr);
        };
        m_pTaskScheduler.reset(new enki::TaskScheduler());
        m_pTaskScheduler->Initialize(tsConfig);

        m_WndHandle = windowHandle;

        eastl::string configFile = m_WorkingPath + "Config/VultanaEngine.ini";
        CSimpleIniA configIni;
        if (configIni.LoadFile(configFile.c_str()) < 0)
        {
            VTNA_LOG_ERROR("Failed to load config file: {}", configFile);
        }

        m_AssetsPath = configIni.GetValue("Vultana", "AssetsPath");
        m_ShaderPath = configIni.GetValue("Vultana", "ShaderPath");

        RHI::ERHIRenderBackend renderBackend = RHI::ERHIRenderBackend::Vulkan;

        m_pRenderer = eastl::make_unique<Renderer::RendererBase>();
        if (!m_pRenderer->CreateDevice(renderBackend, m_WndHandle, width, height))
        {
            VTNA_LOG_ERROR("Failed to create renderer device");
            exit(0);
        }

        m_pWorld = eastl::make_unique<Scene::World>();
        m_pWorld->LoadScene(m_AssetsPath + configIni.GetValue("World", "SceneFile"));

        m_pEditor = eastl::make_unique<Editor::VultanaEditor>(m_pRenderer.get());

        stm_setup();
    }

    void VultanaEngine::Shutdown()
    {
        m_pTaskScheduler->WaitforAll();

        m_pWorld.reset();
        m_pEditor.reset();
        
        m_pTaskScheduler.reset();

        m_pRenderer.reset();

        spdlog::shutdown();
    }

    void VultanaEngine::Tick()
    {
        m_FrameTime = (float)stm_sec(stm_laptime(&m_LastFrameTime));

        m_pEditor->NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        bool isMinimized = io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f;

        if (isMinimized)
        {
            ImGui::Render();
        }
        else
        {
            m_pEditor->Tick();
            m_pWorld->Tick(m_FrameTime);
            m_pRenderer->RenderFrame();
        }
    }
    
    VultanaEngine::~VultanaEngine()
    {
        rpmalloc_finalize();
    }
}