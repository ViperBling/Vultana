#pragma once

#include "Scene/ECS/AssetRegistry.hpp"
#include <EASTL/string.h>

namespace Assets
{
    class FModelLoaderECS
    {
    public:
        static bool LoadModel(const eastl::string& filePath, Scene::ECS::FModelAssetInfo& outInfo);
    };
} // namespace Assets