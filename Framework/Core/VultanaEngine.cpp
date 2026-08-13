#include "Core/VultanaEngine.hpp"

#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetManager/AssetManager.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "AssetManager/SceneAssetRegistry.hpp"

#include "Editor/VultanaEditor.hpp"

#include "Renderer/ECSRenderBridge.hpp"
#include "Scene/Camera.hpp"
#include "Scene/ECS/Processors/CullingProcessor.hpp"
#include "Scene/ECS/Processors/HierarchyProcessor.hpp"
#include "Scene/ECS/Processors/RenderSubmissionProcessor.hpp"
#include "Scene/SceneComponent/Lights/Light.hpp"

#include "Utilities/Log.hpp"
#include "Utilities/String.hpp"
#include "Utilities/ParallelFor.hpp"

#include <EASTL/span.h>
#include <EASTL/string.h>
#include <rpmalloc/rpmalloc.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#define SOKOL_IMPL
#include <sokol/sokol_time.h>
#include <ImGui/imgui.h>
#include <SimpleIni.h>
#include <enkiTS/TaskScheduler.h>
#include <functional>

namespace Core
{
    void ParallelCullingImpl(uint32_t count, const std::function<void(uint32_t)>& func)
    {
        // Engine layer owns the parallel implementation; the ECS core stays engine-free.
        Utilities::ParallelFor(count, func);
    }

    FVultanaEngine *FVultanaEngine::GetEngineInstance()
    {
        static FVultanaEngine engine;
        return &engine;
    }

    void FVultanaEngine::Init(void* windowHandle, uint32_t width, uint32_t height)
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
        const char* sceneBackend = configIni.GetValue("World", "SceneBackend", "Legacy");
        m_bUseECSBackend = (strcmp(sceneBackend, "ECS") == 0);

        Assets::FAssetPathUtils::SetAssetsRootPath(m_AssetsPath);
        Assets::FAssetPathUtils::SetContentRootPath(configIni.GetValue("Vultana", "ContentPath", "../Content/"));

        RHI::ERHIRenderBackend renderBackend = RHI::ERHIRenderBackend::Vulkan;

        m_pRenderer = eastl::make_unique<Renderer::FRendererBase>();
        if (!m_pRenderer->CreateDevice(renderBackend, m_WndHandle, width, height))
        {
            VTNA_LOG_ERROR("Failed to create renderer device");
            exit(0);
        }

        m_pRenderer->RegisterFrameHook(Renderer::ERenderPhase::BeforeUpload,
            [this]() { m_pAssetManager->DrainReadyQueue(); });

        m_pEntitySystem = eastl::make_unique<Scene::ECS::FEntitySubsystem>();
        m_pAssetRegistry = eastl::make_unique<Assets::FSceneAssetRegistry>(m_pRenderer.get());
        m_pRenderSink = eastl::make_unique<Renderer::FECSRenderBridge>(m_pRenderer.get(), static_cast<Assets::FSceneAssetRegistry*>(m_pAssetRegistry.get()));
        m_pEntityFactory = eastl::make_unique<Scene::ECS::FEntityFactory>(*m_pEntitySystem, *m_pAssetRegistry);

        m_pWorld = eastl::make_unique<Scene::FWorld>();
        const char* modelLoadPath = configIni.GetValue("World", "ModelLoadPath", "Asset");
        m_pWorld->SetUseCookedAssets(strcmp(modelLoadPath, "Asset") == 0);
        m_pWorld->SetUseECSBackend(m_bUseECSBackend);
        m_pAssetManager = eastl::make_unique<Assets::FAssetManager>(m_pWorld.get(), m_pTaskScheduler.get());
        m_pWorld->LoadScene(m_AssetsPath + configIni.GetValue("World", "SceneFile"));

        if (m_bUseECSBackend)
        {
            m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FHierarchyProcessor>());
            m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FCullingProcessor>(eastl::span<const float4>(m_PendingCullingPlanes, 6), &ParallelCullingImpl));
            m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FRenderSubmissionProcessor>(*m_pRenderSink, *m_pAssetRegistry));
        }

        m_pEditor = eastl::make_unique<Editor::FVultanaEditor>(m_pRenderer.get());

        stm_setup();
    }

    void FVultanaEngine::Shutdown()
    {
        m_pTaskScheduler->WaitforAll();

        m_pWorld.reset();
        m_pEditor.reset();

        m_pEntityFactory.reset();
        m_pRenderSink.reset();
        m_pAssetRegistry.reset();
        m_pEntitySystem.reset();

        m_pTaskScheduler.reset();
        m_pAssetManager.reset();

        Assets::FResourceCache::GetInstance()->ReleaseAllTextures();

        m_pRenderer.reset();

        spdlog::shutdown();
    }

    void FVultanaEngine::Tick()
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
            if (m_pAssetManager->HasPendingRequests())
            {
                ImGui::Begin("Asset Loading", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::Text("Loading assets... %u pending", m_pAssetManager->GetPendingRequestCount());
                ImGui::End();
                m_pEditor->Tick();
                m_pRenderer->RenderLoadingFrame();
            }
            else
            {
                m_pEditor->Tick();
                m_pWorld->Tick(m_FrameTime);
                UpdateSceneRenderState();
                if (m_bUseECSBackend)
                {
                    m_pEntitySystem->Tick(m_FrameTime);
                }
                m_pRenderer->RenderFrame();
            }
            m_pAssetRegistry->FrameFencePassed(m_pRenderer->GetFrameID());
        }
    }
    
    FVultanaEngine::~FVultanaEngine()
    {
        rpmalloc_finalize();
    }

    void FVultanaEngine::UpdateSceneRenderState()
    {
        if (m_pWorld == nullptr) return;

        Scene::FCamera* pCamera = m_pWorld->GetCamera();
        if (pCamera == nullptr) return;

        memcpy(m_PendingCullingPlanes, pCamera->GetFrustumPlanes(), sizeof(m_PendingCullingPlanes));

        Renderer::FSceneRenderState state;
        pCamera->SetupCameraCB(state.CameraConstants);
        state.bCameraValid = true;
        if (Scene::ILight* pMainLight = m_pWorld->FindMainLight())
        {
            state.LightColor = pMainLight->GetLightColor();
            state.LightDirection = pMainLight->GetLightDirection();
            state.LightRadius = pMainLight->GetLightRadius();
            state.bLightValid = true;
        }
        else
        {
            state.bLightValid = false;
        }
        m_pRenderer->SetSceneRenderState(state);
    }
} // namespace Core