#include "ModelLoaderECS.hpp"
#include "AssetManager/AssetManager.hpp"
#include "AssetManager/SceneAssetRegistry.hpp"
#include "Scene/ECS/EntityFactory.hpp"
#include "Core/VultanaEngine.hpp"
#include "Utilities/Log.hpp"

namespace Assets
{
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
} // namespace Assets