#include "AssetManager/SceneAssetRegistry.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "Utilities/Log.hpp"
#include "SceneAssetRegistry.hpp"

namespace Assets
{
    FSceneAssetRegistry::FSceneAssetRegistry(Renderer::FRendererBase* pRenderer) : m_pRenderer(pRenderer)
    {
        VTNA_LOG_INFO("SceneAssetRegistry created");
    }

    FSceneAssetRegistry::~FSceneAssetRegistry() {}

    Scene::ECS::FModelAssetHandle FSceneAssetRegistry::ImportModel(const eastl::string& assetFile)
    {
        for (uint32_t i = 0; i < m_Assets.size(); ++i)
        {
            if (m_Assets[i].Generation > 0 && m_Assets[i].ModelPath == assetFile)
                return { i, m_Assets[i].Generation };
        }
        m_Assets.emplace_back();
        m_Assets.back().Generation = 1;
        m_Assets.back().ModelPath = assetFile;
        VTNA_LOG_INFO("SceneAssetRegistry : Imported model {}", assetFile);
        return { static_cast<uint32_t>(m_Assets.size() - 1), 1 };
    }

    Scene::ECS::FMeshResourceHandle FSceneAssetRegistry::AcquireMesh(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx)
    {
        if (asset.Index >= m_Assets.size() || m_Assets[asset.Index].Generation != asset.Generation) 
            return {};

        auto& a = m_Assets[asset.Index];
        if (a.MeshCount == 0) 
            a.FirstMesh = static_cast<uint32_t>(m_Meshes.size());

        m_Meshes.emplace_back();
        m_Meshes.back().Generation = asset.Generation;
        m_Meshes.back().RefCount = 1;
        ++a.MeshCount;

        return { static_cast<uint32_t>(m_Meshes.size() - 1), asset.Generation };
    }

    Scene::ECS::FMaterialResourceHandle FSceneAssetRegistry::AcquireMaterial(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx)
    {
        if (asset.Index >= m_Assets.size() || m_Assets[asset.Index].Generation != asset.Generation) 
            return {};

        auto& a = m_Assets[asset.Index];
        if (a.MaterialCount == 0) 
            a.FirstMaterial = static_cast<uint32_t>(m_Materials.size());

        m_Materials.emplace_back();
        m_Materials.back().Generation = asset.Generation;
        m_Materials.back().RefCount = 1;
        ++a.MaterialCount;

        return { static_cast<uint32_t>(m_Materials.size() - 1), asset.Generation };
    }

    void FSceneAssetRegistry::Release(Scene::ECS::FMeshResourceHandle handle)
    {
    }

    void FSceneAssetRegistry::Release(Scene::ECS::FMaterialResourceHandle handle)
    {
    }

    Scene::ECS::FMeshGPUData FSceneAssetRegistry::GetMeshData(Scene::ECS::FMeshResourceHandle handle) const
    {
        if (handle.Index < m_Meshes.size() && m_Meshes[handle.Index].Generation == handle.Generation)
            return m_Meshes[handle.Index].GPUData;
        return {};
    }

    Scene::ECS::FMaterialGPUData FSceneAssetRegistry::GetMaterialData(Scene::ECS::FMaterialResourceHandle handle) const
    {
        if (handle.Index < m_Materials.size() && m_Materials[handle.Index].Generation == handle.Generation)
            return m_Materials[handle.Index].GPUData;
        return {};
    }

    Scene::ECS::FModelAssetInfo FSceneAssetRegistry::GetModelInfo(Scene::ECS::FModelAssetHandle handle) const
    {
        Scene::ECS::FModelAssetInfo info;
        if (handle.Index < m_Assets.size() && m_Assets[handle.Index].Generation == handle.Generation)
        {
            info.ModelPath = m_Assets[handle.Index].ModelPath;
        }
        return info;
    }

    void FSceneAssetRegistry::FrameFencePassed(uint64_t frameID)
    {
    }

    RHI::FRHIPipelineState* FSceneAssetRegistry::GetMeshletPSO(Scene::ECS::FMaterialResourceHandle handle) const
    {
        if (handle.Index < m_Materials.size() && m_Materials[handle.Index].Generation == handle.Generation)
        {
            if (m_Materials[handle.Index].Material)
                return m_Materials[handle.Index].Material->GetMeshletPSO();
        }
        return nullptr;
    }

    RHI::FRHIPipelineState* FSceneAssetRegistry::GetIDPSO(Scene::ECS::FMaterialResourceHandle handle) const
    {
        if (handle.Index < m_Materials.size() && m_Materials[handle.Index].Generation == handle.Generation)
        {
            if (m_Materials[handle.Index].Material)
                return m_Materials[handle.Index].Material->GetIDPSO();
        }
        return nullptr;
    }

    RHI::FRHIPipelineState* FSceneAssetRegistry::GetOutlinePSO(Scene::ECS::FMaterialResourceHandle handle) const
    {
        if (handle.Index < m_Materials.size() && m_Materials[handle.Index].Generation == handle.Generation)
        {
            if (m_Materials[handle.Index].Material)
                return m_Materials[handle.Index].Material->GetOutlinePSO();
        }
        return nullptr;
    }
} // namespace Assets
