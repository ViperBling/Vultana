#pragma once

#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/EntityFactory.hpp"
#include "Scene/ECS/AssetRegistry.hpp"

#include <EASTL/vector.h>
#include <EASTL/string.h>

namespace Scene::ECS
{
    struct FModelLoadRequest
    {
        FModelAssetHandle Handle;
        entt::entity Root = entt::null;
        eastl::string FilePath;
        float3 Position = float3(0.0f);
        quaternion Rotation = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        float3 Scale = float3(1.0f);
    };

    struct FModelPreloadRequest
    {
        FModelAssetHandle Handle;
        eastl::string FilePath;
    };

    struct FSceneLoadResult
    {
        bool bHasCamera = false;
        float3 CameraPosition = float3(0.0f);
        float3 CameraRotation = float3(0.0f); // euler degrees
        float CameraFov = 60.0f;
        float CameraZNear = 0.01f;
        eastl::vector<FModelLoadRequest> Models;
        eastl::vector<FModelPreloadRequest> ModelPreloads;
        eastl::vector<FMeshOverride> MeshOverrides;
    };

    class FSceneSerializer
    {
    public:
        // Reads <Scene> (legacy) or <ECSScene>; creates model roots + empty entities
        // + camera/light entities. Async model fills are returned as requests.
        static bool Load(FEntitySubsystem& subsystem, FEntityFactory& factory, IAssetRegistry& assetRegistry, const eastl::string& filePath, const eastl::string& assetsPath, FSceneLoadResult& outResult);
        // Writes the current user-editable entity tree to <ECSScene>.
        static bool Save(FEntitySubsystem& subsystem, IAssetRegistry& assetRegistry, const eastl::string& filePath, const eastl::string& assetsPath);
    };
}