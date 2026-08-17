#include "ModelLoaderECS.hpp"
#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetManager/AssetManager.hpp"
#include "AssetManager/SceneAssetRegistry.hpp"
#include "Scene/ECS/EntityFactory.hpp"
#include "Core/VultanaEngine.hpp"
#include "Utilities/Log.hpp"

#include <EASTL/hash_set.h>

namespace Assets
{
    constexpr float kLoadSpawnDistance = 5.0f;

    void FModelLoaderECS::LoadModelAsync(const eastl::string& filePath, Scene::ECS::FModelAssetHandle assetHandle, Scene::ECS::FEntityFactory& factory, entt::entity rootEntity, const float3& position, const quaternion& rotation, const float3& scale)
    {
        auto* pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pAssetManager = pEngine->GetAssetManager();
        if (pAssetManager == nullptr)
        {
            VTNA_LOG_ERROR("[ModelLoaderECS] no asset manager; cannot load {}", filePath);
            return;
        }

        pAssetManager->RequestModel(filePath,
        [pEngine, &factory, assetHandle, rootEntity, position, rotation, scale](FModelLoadResult&& result)
        {
            auto* registry = static_cast<FSceneAssetRegistry*>(pEngine->GetAssetRegistry());
            if (!result.bSuccess || !result.Payload)
            {
                VTNA_LOG_ERROR("[ModelLoaderECS] failed to load {}", result.SrcPath);
                registry->MarkModelFailed(assetHandle);
                factory.DestroyModelInstance(rootEntity);
                return;
            }

            // Slice 5 (M3): hold the payload and defer PopulateModel + hierarchy creation until
            // every texture ref is ready (all completions land on the main thread). The payload
            // is captured by value (shared_ptr) so it outlives the drain.
            const eastl::string srcPath = result.SrcPath;
            auto payload = result.Payload;
            pEngine->GetAssetManager()->RequestModelTextures(*payload,
            [pEngine, &factory, assetHandle, rootEntity, position, rotation, scale, payload, srcPath]()
            {
                auto* registry = static_cast<FSceneAssetRegistry*>(pEngine->GetAssetRegistry());
                registry->PopulateModel(assetHandle, *payload);

                const Scene::ECS::FModelAssetInfo info = registry->GetModelInfo(assetHandle);
                factory.CreateModelHierarchy(assetHandle, info, rootEntity, position, rotation, scale);
                VTNA_LOG_INFO("[ModelLoaderECS] loaded {} ({} nodes, {} primitives)", srcPath, info.Nodes.size(), info.PrimitiveCount);
            });
        });
    }

    void FModelLoaderECS::PreloadModelAsync(const eastl::string& filePath, Scene::ECS::FModelAssetHandle assetHandle)
    {
        auto* pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pAssetManager = pEngine->GetAssetManager();
        if (pAssetManager == nullptr)
        {
            VTNA_LOG_ERROR("[ModelLoaderECS] preload: no asset manager; cannot preload {}", filePath);
            return;
        }
        pAssetManager->RequestModel(filePath,
        [pEngine, assetHandle](FModelLoadResult&& result)
        {
            auto* registry = static_cast<FSceneAssetRegistry*>(pEngine->GetAssetRegistry());
            if (!result.bSuccess || !result.Payload)
            {
                VTNA_LOG_ERROR("[ModelLoaderECS] preload failed for {}", result.SrcPath);
                registry->MarkModelFailed(assetHandle);
                return;
            }
            auto payload = result.Payload;
            pEngine->GetAssetManager()->RequestModelTextures(*payload,
            [registry, assetHandle, payload]()
            {
                registry->PopulateModel(assetHandle, *payload);
            });
        });
    }

    void FModelLoaderECS::ReloadModelAsync(const eastl::string& filePath)
    {
        auto* pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pAssetManager = pEngine->GetAssetManager();
        auto* pEntitySystem = pEngine->GetEntitySystem();
        auto* factory = pEngine->GetEntityFactory();
        auto* registry = static_cast<FSceneAssetRegistry*>(pEngine->GetAssetRegistry());

        if (pAssetManager == nullptr || pEntitySystem == nullptr || factory == nullptr || registry == nullptr)
        {
            VTNA_LOG_ERROR("[ModelLoaderECS] reload: subsystem missing; cannot reload {}", filePath);
            return;
        }

        static eastl::hash_set<eastl::string> sReloadInFlight;
        if (!sReloadInFlight.insert(filePath).second)
        {
            VTNA_LOG_WARN("[ModelLoaderECS] reload already in flight for {}; request ignored", filePath);
            return;
        }

        Scene::ECS::FModelAssetHandle assetHandle;
        if (!registry->FindModelHandle(filePath, assetHandle))
        {
            VTNA_LOG_ERROR("[ModelLoaderECS] reload: model {} is not registered", filePath);
            sReloadInFlight.erase(filePath);
            return;
        }

        pAssetManager->RequestModelReload(filePath,
        [pEngine, pEntitySystem, registry, factory, assetHandle, filePath](FModelLoadResult&& result)
        {
            if (!result.bSuccess || !result.Payload)
            {
                VTNA_LOG_ERROR("[ModelLoaderECS] reload failed for {}; scene left untouched", result.SrcPath);
                sReloadInFlight.erase(filePath);
                return;
            }

            auto payload = result.Payload;
            pEngine->GetAssetManager()->RequestModelTextures(*payload,
            [pEngine, pEntitySystem, registry, factory, assetHandle, filePath, payload]()
            {
                struct FInstance
                {
                    float3 Position = float3(0.0f);
                    float4 Rotation = float4(0.0f, 0.0f, 0.0f, 1.0f);
                    float3 Scale    = float3(1.0f);
                };
                eastl::vector<FInstance> instances;
                eastl::vector<entt::entity> roots;
                {
                    const auto& reg = pEntitySystem->GetRegistry();
                    auto view = reg.view<Scene::ECS::FModelInstanceComponent>();
                    for (entt::entity e : view)
                    {
                        const auto& comp = view.get<Scene::ECS::FModelInstanceComponent>(e);
                        if (comp.ModelPath != filePath)
                            continue;
                        FInstance inst;
                        if (const auto* tf = reg.try_get<Scene::ECS::FTransformComponent>(e))
                        {
                            inst.Position = tf->Position;
                            inst.Rotation = tf->Rotation;
                            inst.Scale = tf->Scale;
                        }
                        instances.push_back(inst);
                        roots.push_back(e);
                    }
                }

                for (entt::entity root : roots)
                    factory->DestroyModelInstance(root);

                if (!registry->ReloadModel(assetHandle, *payload))
                {
                    VTNA_LOG_ERROR("[ModelLoaderECS] reload: ReloadModel failed for {}", filePath);
                    registry->MarkModelFailed(assetHandle);
                    sReloadInFlight.erase(filePath);
                    return;
                }

                const Scene::ECS::FModelAssetInfo info = registry->GetModelInfo(assetHandle);
                for (const FInstance& inst : instances)
                {
                    entt::entity root = factory->CreateModelInstance(assetHandle, filePath);
                    factory->CreateModelHierarchy(assetHandle, info, root, inst.Position, inst.Rotation, inst.Scale);
                }
                VTNA_LOG_INFO("[ModelLoaderECS] reloaded {} ({} nodes, {} primitives, {} instances)", filePath, info.Nodes.size(), info.PrimitiveCount, instances.size());
                sReloadInFlight.erase(filePath);
            });
        });
    }

    void FModelLoaderECS::ReloadTextureAsync(const eastl::string& productPath)
    {
        auto* pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pAssetManager = pEngine->GetAssetManager();
        auto* registry = static_cast<FSceneAssetRegistry*>(pEngine->GetAssetRegistry());
        if (pAssetManager == nullptr || registry == nullptr)
        {
            VTNA_LOG_ERROR("[ModelLoaderECS] texture reload: subsystem missing; cannot reload {}", productPath);
            return;
        }

        static eastl::hash_set<eastl::string> sTextureReloadInFlight;
        if (!sTextureReloadInFlight.insert(productPath).second)
        {
            VTNA_LOG_WARN("[ModelLoaderECS] texture reload already in flight for {}; request ignored", productPath);
            return;
        }

        pAssetManager->RequestTextureReload(productPath,
        [pEngine, registry, productPath](FTextureLoadResult&& result)
        {
            if (!result.bSuccess || !result.Payload)
            {
                VTNA_LOG_ERROR("[ModelLoaderECS] texture reload failed for {}; scene left untouched", result.SrcPath);
                sTextureReloadInFlight.erase(productPath);
                return;
            }

            registry->ReloadTexture(productPath);
            VTNA_LOG_INFO("[ModelLoaderECS] texture reloaded {}", productPath);
            sTextureReloadInFlight.erase(productPath);
        });
    }

    void FModelLoaderECS::LoadModelAtCamera(const eastl::string& filePath)
    {
        auto* pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* factory = pEngine->GetEntityFactory();
        auto* registry = pEngine->GetAssetRegistry();
        const Renderer::FCameraMathState* camState = pEngine->GetCameraMathState();
        if (factory == nullptr || registry == nullptr || camState == nullptr)
        {
            VTNA_LOG_ERROR("[ModelLoaderECS] load-at-camera: subsystem missing (no ECS camera)");
            return;
        }
        if (!pEngine->IsUseECSBackend())
        {
            VTNA_LOG_WARN("[ModelLoaderECS] load-at-camera requires the ECS scene backend (Config: World/SceneBackend = ECS)");
            return;
        }

        const eastl::string canonical = FAssetPathUtils::MakeEngineAssetPath(filePath);
        if (canonical.empty())
        {
            VTNA_LOG_ERROR("[ModelLoaderECS] load-at-camera: empty model path");
            return;
        }

        const float3 spawnPos = camState->Position + camState->World[2].xyz() * kLoadSpawnDistance;
        const quaternion spawnRot(0.0f, 0.0f, 0.0f, 1.0f);
        const float3 spawnScale(1.0f);

        auto assetHandle = registry->ImportModel(canonical);

        if (registry->IsReady(assetHandle))
        {
            const Scene::ECS::FModelAssetInfo info = registry->GetModelInfo(assetHandle);
            entt::entity root = factory->CreateModelInstance(assetHandle, canonical);
            factory->CreateModelHierarchy(assetHandle, info, root, spawnPos, spawnRot, spawnScale);
            VTNA_LOG_INFO("[ModelLoaderECS] load-at-camera: {} already resident; new instance only ({} nodes)", canonical, info.Nodes.size());
            return;
        }

        entt::entity root = factory->CreateModelInstance(assetHandle, canonical);
        LoadModelAsync(canonical, assetHandle, *factory, root, spawnPos, spawnRot, spawnScale);
    }

    entt::entity FModelLoaderECS::FindModelInstanceRoot(const entt::registry& reg, entt::entity start)
    {
        entt::entity e = start;
        while (reg.valid(e))
        {
            if (reg.try_get<Scene::ECS::FModelInstanceComponent>(e) != nullptr)
                return e;
            const Scene::ECS::FParentComponent* parent = reg.try_get<Scene::ECS::FParentComponent>(e);
            if (parent == nullptr || parent->Parent == entt::null)
                break;
            e = parent->Parent;
        }
        return entt::null;
    }
} // namespace Assets