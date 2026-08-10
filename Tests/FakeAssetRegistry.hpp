#pragma once 

#include "Scene/ECS/AssetRegistry.hpp"
#include <EASTL/hash_map.h>
#include <EASTL/vector.h>

class FFakeAssetRegistry final : public Scene::ECS::IAssetRegistry
{
public:
    struct FMeshEntry
    {
        uint32_t AcquireCount = 0;
        Scene::ECS::FMeshGPUData GPUData;
    };
    struct FMaterialEntry
    {
        uint32_t AcquireCount = 0;
    };
    struct FModelEntry
    {
        eastl::string ModelPath;
        uint32_t ImportCount = 0;
        eastl::vector<FMeshEntry> Meshes;
        eastl::vector<FMaterialEntry> Materials;
    };

    Scene::ECS::FModelAssetHandle ImportModel(const eastl::string& assetFile) override
    {
        for (uint32_t i = 0; i < m_Models.size(); ++i)
        {
            if (m_Models[i].ModelPath == assetFile) return {i, 1};
        }
        m_Models.emplace_back();
        m_Models.back().ModelPath = assetFile;
        m_Models.back().ImportCount = 1;
        return {static_cast<uint32_t>(m_Models.size() - 1), 1};
    }

    Scene::ECS::FMeshResourceHandle AcquireMesh(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx) override
    {
        if (asset.Index >= m_Models.size()) return {};
        auto& meshes = m_Models[asset.Index].Meshes;
        meshes.emplace_back();
        meshes.back().AcquireCount = 1;
        meshes.back().GPUData.MeshletCount = 1;
        return { static_cast<uint32_t>(meshes.size() - 1), 1 };
    }

    Scene::ECS::FMaterialResourceHandle AcquireMaterial(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx) override
    {
        if (asset.Index >= m_Models.size()) return {};
        auto& materials = m_Models[asset.Index].Materials;
        materials.emplace_back();
        materials.back().AcquireCount = 1;
        return { static_cast<uint32_t>(materials.size() - 1), 1 };
    }

    void Release(Scene::ECS::FMeshResourceHandle) override {}
    void Release(Scene::ECS::FMaterialResourceHandle) override {}

    Scene::ECS::FMeshGPUData GetMeshData(Scene::ECS::FMeshResourceHandle handle) const override
    {
        for (const auto& model : m_Models)
            if (handle.Index < model.Meshes.size() && model.Meshes[handle.Index].GPUData.MeshletCount > 0)
                return model.Meshes[handle.Index].GPUData;
        return {};
    }
    
    Scene::ECS::FMaterialGPUData GetMaterialData(Scene::ECS::FMaterialResourceHandle) const override { return {}; }

    Scene::ECS::FModelAssetInfo GetModelInfo(Scene::ECS::FModelAssetHandle handle) const override
    {
        Scene::ECS::FModelAssetInfo info;
        if (handle.Index < m_Models.size()) 
            info.ModelPath = m_Models[handle.Index].ModelPath;
        return info;
    }

    void FrameFencePassed(uint64_t) override {}

    Scene::ECS::EAssetState GetState(Scene::ECS::FModelAssetHandle) const override
    {
        return Scene::ECS::EAssetState::Ready;
    }

    uint32_t GetMeshAcquireCount(uint32_t assetIdx) const
    {
        if (assetIdx < m_Models.size()) 
            return static_cast<uint32_t>(m_Models[assetIdx].Meshes.size());
        return 0;
    }

private:
    eastl::vector<FModelEntry> m_Models;
};