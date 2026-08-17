#include "Core/VultanaEngine.hpp"

#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetManager/AssetManager.hpp"
#include "AssetManager/ModelLoaderECS.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "AssetManager/SceneAssetRegistry.hpp"

#include "Editor/VultanaEditor.hpp"

#include "Renderer/ECSRenderBridge.hpp"

#include "Scene/ECS/SceneSerializer.hpp"
#include "Scene/ECS/Processors/AnimationProcessor.hpp"
#include "Scene/ECS/Processors/CullingProcessor.hpp"
#include "Scene/ECS/Processors/HierarchyProcessor.hpp"
#include "Scene/ECS/Processors/RenderSubmissionProcessor.hpp"

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
        Utilities::ParallelFor(count, func, 512);
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

        m_pAssetManager = eastl::make_unique<Assets::FAssetManager>(m_pTaskScheduler.get(), true);
        m_SceneFilePath = m_AssetsPath + configIni.GetValue("World", "SceneFile");
        LoadSceneFromFile(m_SceneFilePath);

        m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FAnimationProcessor>(m_pAssetRegistry.get()));
        m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FHierarchyProcessor>());
        m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FCullingProcessor>(eastl::span<const float4>(m_PendingCullingPlanes, 6), &ParallelCullingImpl));
        m_pEntitySystem->RegisterProcessor(eastl::make_unique<Scene::ECS::FRenderSubmissionProcessor>(*m_pRenderSink, *m_pAssetRegistry));

        auto pLight = eastl::make_unique<Scene::ECS::FLightProcessor>(&m_LightSnapshot);
        m_pLightProcessor = pLight.get();
        m_pEntitySystem->RegisterProcessor(eastl::move(pLight));

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
        m_pECSCameraController = eastl::make_unique<FCameraController>();

        m_pEditor = eastl::make_unique<Editor::FVultanaEditor>(m_pRenderer.get());

        stm_setup();
    }

    void FVultanaEngine::Shutdown()
    {
        m_pTaskScheduler->WaitforAll();

        m_pEditor.reset();
        m_pEntityFactory.reset();
        m_pRenderSink.reset();
        m_pAssetRegistry.reset();
        m_pEntitySystem.reset();

        m_pTaskScheduler.reset();
        m_pAssetManager.reset();
        m_pECSCameraController.reset();

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
                m_pECSCameraController->SetAspect(static_cast<float>(m_pRenderer->GetRenderWidth()) / static_cast<float>(m_pRenderer->GetRenderHeight()));
                m_pECSCameraController->Tick(m_FrameTime, *m_pEntitySystem);
                UpdateCullingData();
                if (!m_PendingMeshOverrides.empty())
                {
                    ApplyPendingMeshOverrides();
                }
                m_pEntitySystem->Tick(m_FrameTime);
                UpdateSceneRenderState();
                m_pRenderer->RenderFrame();
            }
        }
    }

    void FVultanaEngine::LoadSceneFromFile(const eastl::string& scenePath)
    {
        if (m_pEntityFactory)
            m_pEntityFactory->DestroyAllModelInstances();
        m_pEntitySystem->Clear();
        if (m_pEditor)
            m_pEditor->ClearEditHistory();
        m_PendingMeshOverrides.clear();

        Scene::ECS::FSceneLoadResult sceneResult;
        Scene::ECS::FSceneSerializer::Load(*m_pEntitySystem, *m_pEntityFactory, *m_pAssetRegistry, scenePath, m_AssetsPath, sceneResult);

        for (const auto& request : sceneResult.Models)
        {
            Assets::FModelLoaderECS::LoadModelAsync(request.FilePath, request.Handle, *m_pEntityFactory, request.Root, request.Position, request.Rotation, request.Scale);
        }

        eastl::vector<Scene::ECS::FModelAssetHandle> scheduledHandles;
        for (const auto& request : sceneResult.Models)
            scheduledHandles.push_back(request.Handle);
        for (const auto& preload : sceneResult.ModelPreloads)
        {
            bool bAlreadyScheduled = false;
            for (const Scene::ECS::FModelAssetHandle& handle : scheduledHandles)
            {
                if (handle == preload.Handle) { bAlreadyScheduled = true; break; }
            }
            if (!bAlreadyScheduled)
            {
                scheduledHandles.push_back(preload.Handle);
                Assets::FModelLoaderECS::PreloadModelAsync(preload.FilePath, preload.Handle);
            }
        }
        m_PendingMeshOverrides = sceneResult.MeshOverrides;

        if (sceneResult.bHasCamera)
        {
            auto& reg = m_pEntitySystem->GetRegistry();
            auto cameraView = reg.view<Scene::ECS::FTransformComponent, Scene::ECS::FCameraComponent, Scene::ECS::FMainCameraTag>();
            if (const entt::entity cam = cameraView.front(); cam != entt::null)
            {
                if (auto* tf = reg.try_get<Scene::ECS::FTransformComponent>(cam))
                {
                    tf->Position = sceneResult.CameraPosition;
                    tf->Rotation = RotationQuat(sceneResult.CameraRotation);
                    m_pEntitySystem->MarkTransformDirty(cam);
                }
                if (auto* camComp = reg.try_get<Scene::ECS::FCameraComponent>(cam))
                {
                    camComp->Fov = sceneResult.CameraFov;
                    camComp->ZNear = sceneResult.CameraZNear;
                }
            }
        }

        // Camera fallback (review C1-6, same as C1.6): Clear() destroyed every entity, so a
        // scene without a camera must get one here or the ECS would permanently lose it.
        if (m_pEntityFactory != nullptr)
        {
            auto& reg = m_pEntitySystem->GetRegistry();
            if (reg.view<Scene::ECS::FMainCameraTag>().front() == entt::null)
            {
                entt::entity cameraEntity = m_pEntityFactory->CreateEmpty("MainCamera");
                reg.emplace<Scene::ECS::FCameraComponent>(cameraEntity);
                reg.emplace<Scene::ECS::FMainCameraTag>(cameraEntity);
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
        Renderer::FSceneRenderState state;
        // IsValid() guard: with no bound camera entity, FillCameraConstants would feed
        // Inverse(zero) NaN/Inf matrices and zero planes would make culling reject nothing.
        if (m_pECSCameraController != nullptr && m_pECSCameraController->IsValid())
        {
            Renderer::FillCameraConstants(m_pECSCameraController->GetState(), state.CameraConstants);
            state.bCameraValid = true;
        }

        const Scene::ECS::FSceneLightSnapshot* lightSnap = (m_pLightProcessor != nullptr) ? m_pLightProcessor->GetOutput() : nullptr;
        if (lightSnap != nullptr && lightSnap->bValid)
        {
            state.LightColor = lightSnap->Color;
            state.LightDirection = lightSnap->Direction;
            state.LightRadius = lightSnap->Radius;
            state.bLightValid = true;
        }
        else
        {
            state.bLightValid = false;
        }

        m_pRenderer->SetSceneRenderState(state);
    }

    void FVultanaEngine::UpdateCullingData()
    {
        if (m_pECSCameraController == nullptr)
            return;
        const Renderer::FCameraMathState& cameraState = m_pECSCameraController->GetState();
        memcpy(m_PendingCullingPlanes, cameraState.FrustumPlanes, sizeof(m_PendingCullingPlanes));
    }

    void FVultanaEngine::ApplyPendingMeshOverrides()
    {
        if (m_pEntityFactory == nullptr || m_pEntitySystem == nullptr)
        {
            m_PendingMeshOverrides.clear();
            return;
        }
        for (const Scene::ECS::FMeshOverride& meshOverride : m_PendingMeshOverrides)
            m_pEntityFactory->ApplyMeshOverride(meshOverride);
        m_PendingMeshOverrides.clear();
    }

} // namespace Core