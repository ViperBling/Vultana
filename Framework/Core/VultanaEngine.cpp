#include "Core/VultanaEngine.hpp"

#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetManager/AssetManager.hpp"
#include "AssetManager/ModelLoaderECS.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "AssetManager/SceneAssetRegistry.hpp"

#include "Editor/VultanaEditor.hpp"

#include "Renderer/ECSRenderBridge.hpp"

#include "Scene/Camera.hpp"
#include "Scene/ECS/SceneSerializer.hpp"
#include "Scene/ECS/Processors/AnimationProcessor.hpp"
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
#include <EASTL/functional.h>

namespace Core
{
    void ParallelCullingImpl(uint32_t count, const eastl::function<void(uint32_t)>& func)
    {
        // Engine layer owns the parallel implementation; the ECS core stays engine-free.
        // For now, simple scene culling is not a performance bottleneck, so we can use a small grain size to reduce overhead.
        Utilities::ParallelFor(count, func, 32);
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
        // m_pWorld->LoadScene(m_AssetsPath + configIni.GetValue("World", "SceneFile"));
        m_SceneFilePath = m_AssetsPath + configIni.GetValue("World", "SceneFile");
        LoadSceneFromFile(m_SceneFilePath);

        if (m_bUseECSBackend)
        {
            m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FAnimationProcessor>(m_pAssetRegistry.get()));
            m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FHierarchyProcessor>());
            m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FCullingProcessor>(eastl::span<const float4>(m_PendingCullingPlanes, 6), &ParallelCullingImpl));
            m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FRenderSubmissionProcessor>(*m_pRenderSink, *m_pAssetRegistry));

            auto pLight = eastl::make_unique<Scene::ECS::FLightProcessor>(&m_LightSnapshot);
            m_pLightProcessor = pLight.get();
            auto pCameraSync = eastl::make_unique<Scene::ECS::FCameraSyncProcessor>(&m_CameraSnapshot);
            m_pCameraSyncProcessor = pCameraSync.get();
            m_pEntitySystem->RegisterProcessor(eastl::move(pLight));
            m_pEntitySystem->RegisterProcessor(eastl::move(pCameraSync));

            auto& reg = m_pEntitySystem->GetRegistry();

            const bool bHasCamera = (reg.view<Scene::ECS::FMainCameraTag>().front() != entt::null);
            const bool bHasLight = (reg.view<Scene::ECS::FMainLightTag>().front() != entt::null);

            if (!bHasCamera)
            {
                entt::entity cameraEntity = m_pEntityFactory->CreateEmpty("MainCamera");
                reg.emplace<Scene::ECS::FCameraComponent>(cameraEntity);
                reg.emplace<Scene::ECS::FMainCameraTag>(cameraEntity);
            }
            if (!bHasLight)
            {
                entt::entity lightEntity = m_pEntityFactory->CreateEmpty("MainLight");
                reg.emplace<Scene::ECS::FLightComponent>(lightEntity);
                reg.emplace<Scene::ECS::FMainLightTag>(lightEntity);
            }
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

        m_pLightProcessor = nullptr;
        m_pCameraSyncProcessor = nullptr;

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
                UpdateCullingData();
                if (m_bUseECSBackend)
                {
                    m_pEntitySystem->Tick(m_FrameTime);
                }
                UpdateSceneRenderState();
                m_pRenderer->RenderFrame();
            }
            m_pAssetRegistry->FrameFencePassed(m_pRenderer->GetFrameID());
        }
    }

    void FVultanaEngine::LoadSceneFromFile(const eastl::string& scenePath)
    {
        if (!m_bUseECSBackend)
        {
            m_pWorld->LoadScene(scenePath);
            return;
        }

        m_pEntitySystem->Clear();
        if (m_pEditor)
            m_pEditor->ClearEditHistory();

        Scene::ECS::FSceneLoadResult sceneResult;
        Scene::ECS::FSceneSerializer::Load(*m_pEntitySystem, *m_pEntityFactory, *m_pAssetRegistry, scenePath, m_AssetsPath, sceneResult);
        
        for (const auto& request : sceneResult.Models)
        {
            Assets::FModelLoaderECS::LoadModelAsync(request.FilePath, request.Handle, *m_pEntityFactory, request.Root, request.Position, request.Rotation, request.Scale);
        }
        if (sceneResult.bHasCamera)
        {
            Scene::FCamera* pCamera = m_pWorld->GetCamera();
            if (pCamera)
            {
                pCamera->SetPosition(sceneResult.CameraPosition);
                pCamera->SetRotation(sceneResult.CameraRotation);
                pCamera->SetPerspective(static_cast<float>(m_pRenderer->GetRenderWidth()) / static_cast<float>(m_pRenderer->GetRenderHeight()), sceneResult.CameraFov, sceneResult.CameraZNear);
            }
        }
        m_SceneFilePath = scenePath;
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

        Renderer::FSceneRenderState state;
        pCamera->SetupCameraCB(state.CameraConstants);
        state.bCameraValid = true;

        const Scene::ECS::FSceneLightSnapshot* lightSnap = (m_pLightProcessor != nullptr) ? m_pLightProcessor->GetOutput() : nullptr;
        if (m_bUseECSBackend && lightSnap != nullptr && lightSnap->bValid)
        {
            state.LightColor = lightSnap->Color;
            state.LightDirection = lightSnap->Direction;
            state.LightRadius = lightSnap->Radius;
            state.bLightValid = true;
        }
        else if (Scene::ILight* pMainLight = m_pWorld->FindMainLight())
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

        const Scene::ECS::FSceneCameraSnapshot* camSnap = (m_pCameraSyncProcessor != nullptr) ? m_pCameraSyncProcessor->GetOutput() : nullptr;
        if (m_bUseECSBackend && camSnap != nullptr && camSnap->bValid && m_pEntitySystem != nullptr)
        {
            auto& reg = m_pEntitySystem->GetRegistry();
            if (reg.valid(camSnap->CameraEntity))
            {
                if (auto* tf = reg.try_get<Scene::ECS::FTransformComponent>(camSnap->CameraEntity))
                {
                    tf->Position = pCamera->GetPosition();
                    tf->Rotation = RotationQuat(pCamera->GetRotation()); // degrees -> quaternion
                }
                m_pEntitySystem->MarkTransformDirty(camSnap->CameraEntity);
                if (auto* camComp = reg.try_get<Scene::ECS::FCameraComponent>(camSnap->CameraEntity))
                {
                    camComp->Fov = pCamera->GetFOV();
                    camComp->ZNear = pCamera->GetZNear();
                    camComp->ZFar = pCamera->GetZFar();
                }
            }
        }

        m_pRenderer->SetSceneRenderState(state);
    }

    void FVultanaEngine::UpdateCullingData()
    {
        if (m_pWorld == nullptr)
            return;
        Scene::FCamera* pCamera = m_pWorld->GetCamera();
        if (pCamera == nullptr)
            return;
        memcpy(m_PendingCullingPlanes, pCamera->GetFrustumPlanes(), sizeof(m_PendingCullingPlanes));
    }

} // namespace Core