#include "ModelLoaderECS.hpp"
#include "Utilities/Log.hpp"

namespace Assets
{
    bool FModelLoaderECS::LoadModel(const eastl::string& filePath, Scene::ECS::FModelAssetInfo& outInfo)
    {
        VTNA_LOG_INFO("ModelLoaderECS: Loading model {}", filePath);
        return true;
    }
} // namespace Assets