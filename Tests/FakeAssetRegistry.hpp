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
        int32_t RefCount = 0;                    // Slice2.5 T4: reference count for the delayed-release contract
        bool bPendingRelease = false;            // Slice2.5 T4
        uint32_t Generation = 1;                 // Slice2.5 T4: 0 == invalidated by FrameFencePassed
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
        eastl::vector<Scene::ECS::FModelNodeInfo> Nodes;
    };

    bool bFailMaterialAcquire = false;
    bool bFailMeshAcquire = false;
    int32_t MeshReleaseCount = 0;
    int32_t MaterialReleaseCount = 0;

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
        if (bFailMeshAcquire) return {};                             // T3b
        if (asset.Index >= m_Models.size()) return {};
        auto& meshes = m_Models[asset.Index].Meshes;
        if (primitiveIdx < meshes.size())                            // Slice2.5 T4 (G1): dedupe by (asset, primitiveIdx)
        {
            if (meshes[primitiveIdx].Generation == 0)
            {
                meshes[primitiveIdx].Generation = 1;                 // revived slot (mirrors future slot reuse in the real registry)
                meshes[primitiveIdx].RefCount = 1;
                meshes[primitiveIdx].GPUData.MeshletCount = 1;
            }
            else
            {
                meshes[primitiveIdx].RefCount++;
            }
            return { primitiveIdx, meshes[primitiveIdx].Generation };
        }
        if (meshes.size() <= primitiveIdx)
            meshes.resize(primitiveIdx + 1);                         // ids form an index space; intermediate empty slots are harmless
        auto& entry = meshes[primitiveIdx];
        entry.AcquireCount = 1;
        entry.RefCount = 1;
        entry.Generation = 1;
        entry.GPUData.MeshletCount = 1;
        return { primitiveIdx, 1 };
    }

    Scene::ECS::FMaterialResourceHandle AcquireMaterial(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx) override
    {
        if (bFailMaterialAcquire) return {};                         // T3
        if (asset.Index >= m_Models.size()) return {};
        auto& materials = m_Models[asset.Index].Materials;
        materials.emplace_back();
        materials.back().AcquireCount = 1;
        return { static_cast<uint32_t>(materials.size() - 1), 1 };
    }

    void Release(Scene::ECS::FMeshResourceHandle handle) override
    {
        ++MeshReleaseCount;                                          // T3b rollback counter
        FMeshEntry* entry = FindMeshEntry(handle);
        if (entry == nullptr) return;
        if (--entry->RefCount <= 0 && !entry->bPendingRelease)
        {
            entry->bPendingRelease = true;
            m_PendingMeshReleases.push_back(handle.Index);
        }
    }
    void Release(Scene::ECS::FMaterialResourceHandle) override { ++MaterialReleaseCount; }   // T3b rollback counter

    const Scene::ECS::FMeshGPUData* FindMeshData(Scene::ECS::FMeshResourceHandle handle) const override
    {
        // Slice2.5 T4: generation must match - invalidated slots resolve to nullptr.
        const FMeshEntry* entry = FindMeshEntry(handle);
        return (entry != nullptr && entry->Generation == handle.Generation && entry->GPUData.MeshletCount > 0) ? &entry->GPUData : nullptr;
    }

    const Scene::ECS::FMaterialGPUData* FindMaterialData(Scene::ECS::FMaterialResourceHandle handle) const override
    {
        // Return a stable all-zero material block for any valid handle so the render
        // submission flow can run through.
        if (!handle.IsValid()) return nullptr;
        return &m_DefaultMaterial;
    }

    Scene::ECS::FModelAssetInfo GetModelInfo(Scene::ECS::FModelAssetHandle handle) const override
    {
        Scene::ECS::FModelAssetInfo info;
        if (handle.Index < m_Models.size())
        {
            info.ModelPath = m_Models[handle.Index].ModelPath;
            info.Nodes = m_Models[handle.Index].Nodes;
            uint32_t primCount = 0;
            for (const auto& n : info.Nodes)
                primCount += static_cast<uint32_t>(n.Primitives.size());
            info.PrimitiveCount = primCount;
            info.AnimCount = static_cast<uint32_t>(AnimClips.size());
        }
        return info;
    }

    void FrameFencePassed(uint64_t) override
    {
        // Slice2.5 T4: same re-check semantics as FSceneAssetRegistry::FrameFencePassed (B5):
        // a record re-acquired inside the release window must not be invalidated.
        for (uint32_t idx : m_PendingMeshReleases)
        {
            FMeshEntry* entry = FindMeshEntryByIndex(idx);
            if (entry == nullptr) continue;
            entry->bPendingRelease = false;
            if (entry->RefCount > 0) continue;   // re-acquired: keep alive
            entry->Generation = 0;
        }
        m_PendingMeshReleases.clear();
    }

    const Scene::ECS::FAnimationClip* FindAnimation(Scene::ECS::FModelAssetHandle, uint32_t clipIndex) const override
    {
        return (clipIndex < AnimClips.size()) ? &AnimClips[clipIndex] : nullptr;
    }
    const Scene::ECS::FSkeletonInfo* GetSkeletonInfo(Scene::ECS::FModelAssetHandle) const override
    {
        return &SkeletonInfo;
    }

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

    void SetFakeSkinnedMesh(Scene::ECS::FMeshResourceHandle handle, bool bSkinned)
    {
        if (auto* entry = FindMeshEntry(handle))
        {
            entry->GPUData.bSkinned = bSkinned;
            if (bSkinned)
            {
                entry->GPUData.IndexCount = 3;
                entry->GPUData.JointIDBufferAddress = 0x100;
                entry->GPUData.JointWeightBufferAddress = 0x200;
                entry->GPUData.AnimPositionBufferAddress = 0x300;
                entry->GPUData.AnimNormalBufferAddress = 0x400;
                entry->GPUData.AnimTangentBufferAddress = 0x500;
            }
        }
    }

    void SetFakeModelNodes(Scene::ECS::FModelAssetHandle h, const eastl::vector<Scene::ECS::FModelNodeInfo>& nodes)
    {
        if (h.Index < m_Models.size())
            m_Models[h.Index].Nodes = nodes;
    }

    uint32_t GetMaterialConstantAddress(Scene::ECS::FMaterialResourceHandle handle) const override
    {
        return Scene::ECS::INVALID_ADDRESS;
    }

public:
    const FMeshEntry* FindMeshEntry(Scene::ECS::FMeshResourceHandle handle) const
    {
        for (const auto& model : m_Models)
            if (handle.Index < model.Meshes.size())
                return &model.Meshes[handle.Index];
        return nullptr;
    }
    FMeshEntry* FindMeshEntry(Scene::ECS::FMeshResourceHandle handle)
    {
        for (auto& model : m_Models)
            if (handle.Index < model.Meshes.size())
                return &model.Meshes[handle.Index];
        return nullptr;
    }
    FMeshEntry* FindMeshEntryByIndex(uint32_t idx)
    {
        for (auto& model : m_Models)
            if (idx < model.Meshes.size())
                return &model.Meshes[idx];
        return nullptr;
    }

    eastl::vector<FModelEntry> m_Models;
    Scene::ECS::FMaterialGPUData m_DefaultMaterial;
    eastl::vector<uint32_t> m_PendingMeshReleases;
    eastl::vector<Scene::ECS::FAnimationClip> AnimClips;
    Scene::ECS::FSkeletonInfo SkeletonInfo;
};