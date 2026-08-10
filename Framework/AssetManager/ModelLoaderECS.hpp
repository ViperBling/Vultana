#pragma once

#include "Scene/ECS/AssetRegistry.hpp"
#include "Utilities/Math.hpp"

#include <EASTL/string.h>
#include <entt/entt.hpp>

namespace Scene::ECS
{
    class FEntityFactory;
}

namespace Assets
{
    class FModelLoaderECS
    {
    public:
        static void LoadModelAsync(const eastl::string& filePath, Scene::ECS::FModelAssetHandle assetHandle, Scene::ECS::FEntityFactory& factory, entt::entity rootEntity, const float3& position, const quaternion& rotation, const float3& scale);
    };
} // namespace Assets