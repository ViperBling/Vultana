#include "AssetManager/SceneAssetRegistry.hpp"
#include "AssetCooker/VAssetTypes.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "AssetManager/ModelLoader.hpp"

#include "Common/ModelConstants.hlsli"
#include "Utilities/Log.hpp"
#include "SceneAssetRegistry.hpp"

namespace Assets
{
    Scene::ECS::FMeshGPUData BuildMeshGPUData(const eastl::string& cacheKey, const FMeshCookedData& cooked, eastl::vector<OffsetAllocator::Allocation>& outAllocations)
    {
        auto* cache = FResourceCache::GetInstance();
        Scene::ECS::FMeshGPUData data;

        auto allocBuffer = [&](const char* suffix, const void* ptr, uint32_t bytes) -> uint32_t
        {
            if (bytes == 0 || ptr == nullptr) return 0;
            OffsetAllocator::Allocation alloc = cache->GetSceneBuffer(cacheKey + suffix, ptr, bytes);
            outAllocations.push_back(alloc);
            return alloc.offset;
        };

        data.IndexBufferAddress = allocBuffer("_IndexBuffer", cooked.IndexData.data(), (uint32_t)cooked.IndexData.size());
        data.IndexStride = cooked.IndexStride;
        data.IndexCount = cooked.IndexCount;
        data.VertexCount = cooked.VertexCount;
        data.Center = cooked.Center;
        data.Radius = cooked.Radius;
        data.MeshletCount = cooked.MeshletCount;
        data.MeshletBufferAddress = allocBuffer("_MeshletBuffer", cooked.MeshletBounds.data(), (uint32_t)(sizeof(FMeshlet) * cooked.MeshletBounds.size()));
        data.MeshletIndexBufferAddress = allocBuffer("_MeshletIndicesBuffer", cooked.MeshletIndexData.data(), (uint32_t)cooked.MeshletIndexData.size());
        data.MeshletVertexBufferAddress = allocBuffer("_MeshletVertexBuffer", cooked.MeshletVertexData.data(), (uint32_t)cooked.MeshletVertexData.size());

        for (const FMeshCookedData::FStream& stream : cooked.Streams)
        {
            const uint32_t bytes = (uint32_t)stream.Data.size();
            switch (stream.Type)
            {
            case EVertexStreamType::Position:
                data.PositionBufferAddress = allocBuffer("_PositionBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::TexCoord:
                data.TexCoordBufferAddress = allocBuffer("_TexCoordBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::Normal:
                data.NormalBufferAddress = allocBuffer("_NormalBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::Tangent:
                data.TangentBufferAddress = allocBuffer("_TangentBuffer", stream.Data.data(), bytes);
                break;
            default:
                break; // Joints/Weights feed compute skinning; ECS path is static-only for Slice 4
            }
        }
        return data;
    }

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
                return {i, m_Assets[i].Generation};
        }
        m_Assets.emplace_back();
        m_Assets.back().Generation = 1;
        m_Assets.back().ModelPath = assetFile;
        m_Assets.back().State = Scene::ECS::EAssetState::Loading;
        VTNA_LOG_INFO("SceneAssetRegistry : Imported model {}", assetFile);
        return {static_cast<uint32_t>(m_Assets.size() - 1), 1};
    }

    Scene::ECS::FMeshResourceHandle FSceneAssetRegistry::AcquireMesh(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx)
    {
        if (asset.Index >= m_Assets.size() || m_Assets[asset.Index].Generation != asset.Generation)
            return {};

        auto& a = m_Assets[asset.Index];
        if (a.State != Scene::ECS::EAssetState::Ready || primitiveIdx >= a.MeshCount)
            return {};

        const uint32_t idx = a.FirstMesh + primitiveIdx;
        m_Meshes[idx].RefCount++;
        return {idx, asset.Generation};
    }

    Scene::ECS::FMaterialResourceHandle FSceneAssetRegistry::AcquireMaterial(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx)
    {
        if (asset.Index >= m_Assets.size() || m_Assets[asset.Index].Generation != asset.Generation)
            return {};

        auto& a = m_Assets[asset.Index];
        if (a.State != Scene::ECS::EAssetState::Ready || primitiveIdx >= a.MaterialCount)
            return {};

        const uint32_t idx = a.FirstMaterial + primitiveIdx;
        m_Materials[idx].RefCount++;
        return {idx, asset.Generation};
    }

    void FSceneAssetRegistry::Release(Scene::ECS::FMeshResourceHandle handle)
    {
        if (handle.Index >= m_Meshes.size() || m_Meshes[handle.Index].Generation != handle.Generation)
            return;
        FMeshRecord& rec = m_Meshes[handle.Index];
        rec.RefCount--;
        if (rec.RefCount <= 0 && !rec.bPendingRelease)
        {
            rec.bPendingRelease = true;
            const uint64_t currentFrame = m_pRenderer ? m_pRenderer->GetFrameID() : 0;
            FPendingRelease pending;
            pending.Type = FPendingRelease::EType::Mesh;
            pending.RecordIndex = handle.Index;
            pending.TargetFrame = currentFrame + RHI::RHI_MAX_INFLIGHT_FRAMES;
            m_PendingReleaseQueue.push_back(pending);
        }
    }

    void FSceneAssetRegistry::Release(Scene::ECS::FMaterialResourceHandle handle)
    {
        if (handle.Index >= m_Materials.size() || m_Materials[handle.Index].Generation != handle.Generation)
            return;
        FMaterialRecord& rec = m_Materials[handle.Index];
        rec.RefCount--;
        if (rec.RefCount <= 0 && !rec.bPendingRelease)
        {
            rec.bPendingRelease = true;
            const uint64_t currentFrame = m_pRenderer ? m_pRenderer->GetFrameID() : 0;
            FPendingRelease pending;
            pending.Type = FPendingRelease::EType::Material;
            pending.RecordIndex = handle.Index;
            pending.TargetFrame = currentFrame + RHI::RHI_MAX_INFLIGHT_FRAMES;
            m_PendingReleaseQueue.push_back(pending);
        }
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
            const FModelAsset& a = m_Assets[handle.Index];
            info.ModelPath = a.ModelPath;
            info.Nodes = a.Nodes;
            info.PrimitiveCount = a.PrimitiveCount;
        }
        return info;
    }

    Scene::ECS::EAssetState FSceneAssetRegistry::GetState(Scene::ECS::FModelAssetHandle handle) const
    {
        if (handle.Index < m_Assets.size() && m_Assets[handle.Index].Generation == handle.Generation)
            return m_Assets[handle.Index].State;
        return Scene::ECS::EAssetState::Failed;
    }

    void FSceneAssetRegistry::FrameFencePassed(uint64_t frameID)
    {
        for (size_t i = 0; i < m_PendingReleaseQueue.size();)
        {
            FPendingRelease& pending = m_PendingReleaseQueue[i];
            if (frameID < pending.TargetFrame)
            {
                ++i;
                continue;
            }
            if (pending.Type == FPendingRelease::EType::Mesh)
            {
                FMeshRecord& rec = m_Meshes[pending.RecordIndex];
                for (const OffsetAllocator::Allocation& alloc : rec.Allocations)
                    FResourceCache::GetInstance()->ReleaseSceneBuffer(alloc);
                rec.Allocations.clear();
                rec.Generation = 0; // invalidate the handle
                rec.bPendingRelease = false;
            }
            else
            {
                FMaterialRecord& rec = m_Materials[pending.RecordIndex];
                if (rec.Material)
                {
                    delete rec.Material;
                    rec.Material = nullptr;
                }
                rec.Generation = 0;
                rec.bPendingRelease = false;
            }
            m_PendingReleaseQueue.erase_unsorted(m_PendingReleaseQueue.begin() + i);
        }
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
    
    void FSceneAssetRegistry::PopulateModel(Scene::ECS::FModelAssetHandle asset, const FModelCookedData& model)
    {
        if (asset.Index >= m_Assets.size() || m_Assets[asset.Index].Generation != asset.Generation)
            return;
        FModelAsset& modelAsset = m_Assets[asset.Index];

        // ---- Node table ----
        modelAsset.Nodes.clear();
        modelAsset.Nodes.reserve(model.Nodes.size());
        for (size_t n = 0; n < model.Nodes.size(); ++n)
        {
            const FAssetNode& src = model.Nodes[n];
            Scene::ECS::FModelNodeInfo node;
            node.Name = src.Name;
            node.NodeIndex = static_cast<uint32_t>(n);
            node.Translation = src.Translation;
            node.Rotation = src.Rotation;
            node.Scale = src.Scale;
            node.ParentIndex = src.Parent;
            node.MeshIndex = src.Primitives.empty() ? UINT32_MAX : static_cast<uint32_t>(src.Primitives[0]);
            node.bFrontFaceCCW = src.bFrontFaceCCW;
            for (int32_t p : src.Primitives)
                node.Primitives.push_back(static_cast<uint32_t>(p));
            modelAsset.Nodes.push_back(eastl::move(node));
        }
        modelAsset.PrimitiveCount = static_cast<uint32_t>(model.Meshes.size());

        // Per-primitive front-face winding from the first referencing node (mirror flips winding;
        // a primitive shared by nodes with different windings is a degenerate case — last wins).
        eastl::vector<bool> primWinding(model.Meshes.size(), false);
        for (const FAssetNode& srcNode : model.Nodes)
        {
            for (int32_t p : srcNode.Primitives)
            {
                if (p >= 0 && static_cast<size_t>(p) < primWinding.size())
                    primWinding[static_cast<size_t>(p)] = srcNode.bFrontFaceCCW;
            }
        }

        // ---- Mesh + material records (contiguous per asset) ----
        modelAsset.FirstMesh = static_cast<uint32_t>(m_Meshes.size());
        modelAsset.FirstMaterial = static_cast<uint32_t>(m_Materials.size());
        modelAsset.MeshHandles.clear();
        modelAsset.MaterialHandles.clear();
        modelAsset.MeshHandles.reserve(model.Meshes.size());
        modelAsset.MaterialHandles.reserve(model.Meshes.size());

        const eastl::string cacheKey = "Model(" + modelAsset.ModelPath + " "; // same key space as the legacy path
        for (size_t i = 0; i < model.Meshes.size(); ++i)
        {
            const FMeshCookedData& cooked = model.Meshes[i];

            FMeshRecord meshRecord;
            meshRecord.Generation = asset.Generation;
            meshRecord.RefCount = 1; // 基础引用归资产(FModelAsset)所有;AcquireMesh 每次 +1,实体释放回落
            meshRecord.GPUData = BuildMeshGPUData(cacheKey + cooked.Name + ")", cooked, meshRecord.Allocations);
            m_Meshes.push_back(eastl::move(meshRecord));
            modelAsset.MeshHandles.push_back({modelAsset.FirstMesh + static_cast<uint32_t>(i), asset.Generation});

            FMaterialRecord matRecord;
            matRecord.Generation = asset.Generation;
            matRecord.RefCount = 1;
            const FAssetMaterial* assetMat =
                (cooked.MaterialIndex >= 0) ? &model.Materials[static_cast<size_t>(cooked.MaterialIndex)] : nullptr;
            matRecord.Material = FModelLoader::BuildMaterial(assetMat, cooked.bSkeletalAnim, primWinding[i]);
            if (matRecord.Material)
            {
                matRecord.Material->UpdateConstants();
                const FModelMaterialConstants* constants = matRecord.Material->GetMaterialConstants();
                static_assert(sizeof(FModelMaterialConstants) <= sizeof(matRecord.GPUData.Data),
                              "material constants no longer fit the GPUData blob (80 uints)");
                memcpy(matRecord.GPUData.Data, constants, sizeof(FModelMaterialConstants));
            }
            m_Materials.push_back(eastl::move(matRecord));
            modelAsset.MaterialHandles.push_back({modelAsset.FirstMaterial + static_cast<uint32_t>(i), asset.Generation});
        }

        modelAsset.State = Scene::ECS::EAssetState::Ready;
        modelAsset.MeshCount = static_cast<uint32_t>(model.Meshes.size()); // AcquireMesh 依赖(M8 审查 #8)
        modelAsset.MaterialCount = modelAsset.MeshCount;
    }

    void FSceneAssetRegistry::MarkModelFailed(Scene::ECS::FModelAssetHandle asset)
    {
        if (asset.Index < m_Assets.size() && m_Assets[asset.Index].Generation == asset.Generation)
            m_Assets[asset.Index].State = Scene::ECS::EAssetState::Failed;
    }

} // namespace Assets
