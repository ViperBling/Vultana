#pragma once

#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/AssetRegistry.hpp"

namespace Scene::ECS
{
    struct FMeshOverride
    {
        FModelAssetHandle SourceAsset;
        entt::entity SourceRoot = entt::null;
        int32_t SourceNodeIndex = -1;
        int32_t SourcePrimitiveIndex = -1;
        FModelAssetHandle TargetAsset;
        int32_t TargetNodeIndex = -1;
        int32_t TargetPrimitiveIndex = -1;
    };

    class FEntityFactory
    {
    public:
        FEntityFactory(FEntitySubsystem& subsystem, IAssetRegistry& registry);
        ~FEntityFactory();
        entt::entity CreateModelInstance(FModelAssetHandle assetHandle, const eastl::string& filePath, entt::entity parent = entt::null);
        entt::entity CreateEmpty(const char* name = "Entity", entt::entity parent = entt::null);
        void DestroyModelInstance(entt::entity rootEntity);
        void DestroyAllModelInstances();
        void CreateModelHierarchy(FModelAssetHandle assetHandle, const FModelAssetInfo& info, entt::entity rootEntity, const float3& position, const quaternion& rotation, const float3& scale);
        static void SetMeshRenderer(entt::registry& registry, entt::entity entity, FMeshResourceHandle meshHandle, FMaterialResourceHandle materialHandle, const FMeshGPUData& meshData, FModelAssetHandle asset, int32_t nodeIndex, int32_t primitiveIndex, entt::entity skinnedRoot = entt::null);

        static bool ApplyMeshKey(FEntitySubsystem& subsystem, IAssetRegistry& registry, entt::entity entity, FModelAssetHandle asset, int32_t nodeIndex, int32_t primitiveIndex);

        bool ApplyMeshOverride(const FMeshOverride& meshOverride);

    private:
        FEntitySubsystem& m_Subsystem;
        IAssetRegistry& m_Registry;
    };
}