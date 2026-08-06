#pragma once

#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/AssetRegistry.hpp"

namespace Scene::ECS
{
    class FEntityFactory
    {
    public:
        FEntityFactory(FEntitySubsystem& subsystem, IAssetRegistry& registry);
        entt::entity CreateModelInstance(FModelAssetHandle assetHandle, const eastl::string& filePath, entt::entity parent = entt::null);
        entt::entity CreateEmpty(const char* name = "Entity", entt::entity parent = entt::null);
        void DestroyModelInstance(entt::entity rootEntity);

    private:
        FEntitySubsystem& m_Subsystem;
        IAssetRegistry& m_Registry;
    };
}