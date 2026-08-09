#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "AssetCooker/AssetReader.hpp"

#include <EASTL/string.h>

namespace Scene
{
    class FWorld;
}

namespace Assets
{
    class FAssetManager
    {
    public:
        explicit FAssetManager(Scene::FWorld* InWorld) : m_pWorld(InWorld) {}
        ~FAssetManager() = default;

        bool LoadModel(const eastl::string& srcPath, FVAssetHeader& outHeader, FModelCookedData& outPayload);

    private:
        Scene::FWorld* m_pWorld = nullptr;
    };
}