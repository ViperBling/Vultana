#pragma once

#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/AssetRegistry.hpp"

namespace Scene::ECS
{
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

    private:
        FEntitySubsystem& m_Subsystem;
        IAssetRegistry& m_Registry;
    };
}