#include "AssetManager/SceneAssetRegistry.hpp"
#include "AssetCooker/VAssetTypes.hpp"
#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "AssetManager/ModelLoader.hpp"

#include "Common/ModelConstants.hlsli"
#include "Utilities/Log.hpp"

#include <EASTL/algorithm.h>
#include <EASTL/hash_set.h>

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

    const Scene::ECS::FMeshGPUData* FSceneAssetRegistry::FindMeshData(Scene::ECS::FMeshResourceHandle handle) const
    {
        if (handle.Index < m_Meshes.size() && m_Meshes[handle.Index].Generation == handle.Generation)
            return &m_Meshes[handle.Index].GPUData;
        return nullptr;
    }
    const Scene::ECS::FMaterialGPUData* FSceneAssetRegistry::FindMaterialData(Scene::ECS::FMaterialResourceHandle handle) const
    {
        if (handle.Index < m_Materials.size() && m_Materials[handle.Index].Generation == handle.Generation)
            return &m_Materials[handle.Index].GPUData;
        return nullptr;
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
                if (rec.RefCount > 0)
                {
                    rec.bPendingRelease = false;
                    m_PendingReleaseQueue.erase_unsorted(m_PendingReleaseQueue.begin() + i);
                    continue;
                }
                for (const OffsetAllocator::Allocation& alloc : rec.Allocations)
                    FResourceCache::GetInstance()->ReleaseSceneBuffer(alloc);
                rec.Allocations.clear();
                rec.Generation = 0; // invalidate the handle
                rec.bPendingRelease = false;
            }
            else if (pending.Type == FPendingRelease::EType::Material)
            {
                FMaterialRecord& rec = m_Materials[pending.RecordIndex];
                if (rec.RefCount > 0)
                {
                    rec.bPendingRelease = false;
                    m_PendingReleaseQueue.erase_unsorted(m_PendingReleaseQueue.begin() + i);
                    continue;
                }
                if (rec.Material)
                {
                    delete rec.Material;
                    rec.Material = nullptr;
                }
                rec.Generation = 0;
                rec.bPendingRelease = false;
            }
            else
            {
                delete pending.MaterialPtr;
            }
            m_PendingReleaseQueue.erase_unsorted(m_PendingReleaseQueue.begin() + i);
        }
    }

    void FSceneAssetRegistry::DeferDeleteMaterial(FMeshMaterial* material)
    {
        if (material == nullptr)
            return;
        const uint64_t currentFrame = m_pRenderer ? m_pRenderer->GetFrameID() : 0;
        FPendingRelease pending;
        pending.Type = FPendingRelease::EType::MaterialObject;
        pending.MaterialPtr = material;
        pending.TargetFrame = currentFrame + RHI::RHI_MAX_INFLIGHT_FRAMES;
        m_PendingReleaseQueue.push_back(pending);
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
        if (modelAsset.State == Scene::ECS::EAssetState::Ready)
        {
            VTNA_LOG_INFO("[SceneAssetRegistry] {} already populated; skipping re-populate", modelAsset.ModelPath);
            return;
        }

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
        modelAsset.MaterialHandles.reserve(model.Materials.size());

        modelAsset.Materials.clear();
        modelAsset.TextureRefs.clear();
        modelAsset.Materials.reserve(model.Materials.size());
        for (const FAssetMaterial& mat : model.Materials)
        {
            modelAsset.Materials.push_back(mat);
            for (const FAssetTextureRef& ref : mat.Textures)
            {
                if (eastl::find(modelAsset.TextureRefs.begin(), modelAsset.TextureRefs.end(), ref.RelativePath) == modelAsset.TextureRefs.end())
                    modelAsset.TextureRefs.push_back(ref.RelativePath);
            }
        }

        const eastl::string cacheKey = "Model(" + modelAsset.ModelPath + ")@" + eastl::to_string(modelAsset.ReloadCount) + " ";     // same key space as the legacy path
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
            matRecord.AssetMaterialIndex = cooked.MaterialIndex;
            matRecord.bSkeletalAnim = cooked.bSkeletalAnim;
            matRecord.bFrontFaceCCW = primWinding[i];
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

    bool FSceneAssetRegistry::ReloadModel(Scene::ECS::FModelAssetHandle asset, const FModelCookedData& model)
    {
        if (asset.Index >= m_Assets.size() || m_Assets[asset.Index].Generation != asset.Generation)
            return false;
        if (model.Meshes.empty())
        {
            VTNA_LOG_ERROR("[SceneAssetRegistry] reload: payload for {} has no meshes", m_Assets[asset.Index].ModelPath);
            m_Assets[asset.Index].State = Scene::ECS::EAssetState::Failed;
            return false;
        }
        FModelAsset& modelAsset = m_Assets[asset.Index];
        ++modelAsset.ReloadCount;

        // Release base ref.
        for (const Scene::ECS::FMeshResourceHandle& handle : modelAsset.MeshHandles)
            Release(handle);
        for (const Scene::ECS::FMaterialResourceHandle& handle : modelAsset.MaterialHandles)
            Release(handle);
        modelAsset.MeshHandles.clear();
        modelAsset.MaterialHandles.clear();

        modelAsset.State = Scene::ECS::EAssetState::Loading;
        PopulateModel(asset, model);
        return modelAsset.State == Scene::ECS::EAssetState::Ready;
    }

    void FSceneAssetRegistry::EnumerateModels(eastl::vector<FModelStatus>& out) const
    {
        for (const FModelAsset& asset : m_Assets)
        {
            if (asset.Generation == 0) continue;

            FModelStatus status;
            status.ModelPath = asset.ModelPath;
            status.State = asset.State;
            status.PrimitiveCount = asset.PrimitiveCount;
            status.MeshCount = asset.MeshCount;
            status.MaterialCount = asset.MaterialCount;
            status.ReloadCount = asset.ReloadCount;
            for (const Scene::ECS::FMeshResourceHandle& handle : asset.MeshHandles)
            {
                if (handle.Index < m_Meshes.size())
                    status.RefCount += m_Meshes[handle.Index].RefCount;
            }
            out.push_back(eastl::move(status));
        }
    }

    bool FSceneAssetRegistry::FindModelHandle(const eastl::string& modelPath, Scene::ECS::FModelAssetHandle& outHandle) const
    {
        for (uint32_t i = 0; i < m_Assets.size(); ++i)
        {
            if (m_Assets[i].Generation > 0 && m_Assets[i].ModelPath == modelPath)
            {
                outHandle = {i, m_Assets[i].Generation};
                return true;
            }
        }
        return false;
    }

    void FSceneAssetRegistry::ReloadTexture(const eastl::string& productPath)
    {
        const eastl::string wanted = FAssetPathUtils::NormalizeAssetPath(productPath);
        auto refToProduct = [](const eastl::string& rel)
        {
            return FAssetPathUtils::NormalizeAssetPath(FAssetPathUtils::GetContentRootPath() + rel);
        };

        for (FModelAsset& a : m_Assets)
        {
            if (a.Generation == 0)
                continue;
            bool bAssetReferences = false;
            for (const eastl::string& ref : a.TextureRefs)
            {
                if (refToProduct(ref) == wanted)
                {
                    bAssetReferences = true;
                    break;
                }
            }
            if (!bAssetReferences)
                continue;

            for (const Scene::ECS::FMaterialResourceHandle& h : a.MaterialHandles)
            {
                if (h.Index >= m_Materials.size())
                    continue;
                FMaterialRecord& rec = m_Materials[h.Index];
                if (rec.Generation == 0 || rec.bPendingRelease)
                    continue;
                const FAssetMaterial* assetMat =
                    (rec.AssetMaterialIndex >= 0 && static_cast<size_t>(rec.AssetMaterialIndex) < a.Materials.size())
                        ? &a.Materials[static_cast<size_t>(rec.AssetMaterialIndex)]
                        : nullptr;
                if (assetMat == nullptr)
                    continue;

                bool bMatReferences = false;
                for (const FAssetTextureRef& ref : assetMat->Textures)
                {
                    if (refToProduct(ref.RelativePath) == wanted)
                    {
                        bMatReferences = true;
                        break;
                    }
                }
                if (!bMatReferences)
                    continue;

                if (rec.Material)
                {
                    DeferDeleteMaterial(rec.Material);
                    rec.Material = nullptr;
                }
                rec.Material = FModelLoader::BuildMaterial(assetMat, rec.bSkeletalAnim, rec.bFrontFaceCCW);
                if (rec.Material)
                {
                    rec.Material->UpdateConstants();
                    const FModelMaterialConstants* constants = rec.Material->GetMaterialConstants();
                    static_assert(sizeof(FModelMaterialConstants) <= sizeof(rec.GPUData.Data), "material constants no longer fit the GPUData blob (80 uints)");
                    memcpy(rec.GPUData.Data, constants, sizeof(FModelMaterialConstants));
                }
            }
        }
    }

    void FSceneAssetRegistry::EnumerateTextures(eastl::vector<FTextureStatus>& out) const
    {
        eastl::hash_set<eastl::string> seen;
        for (const FModelAsset& a : m_Assets)
        {
            if (a.Generation == 0)
                continue;
            for (const eastl::string& ref : a.TextureRefs)
            {
                if (!seen.insert(ref).second)
                    continue;
                FTextureStatus status;
                status.ProductPath = FAssetPathUtils::GetContentRootPath() + ref;
                status.bResident = FResourceCache::GetInstance()->IsTexture2DProductResident(status.ProductPath);
                out.push_back(eastl::move(status));
            }
        }
    }

} // namespace Assets
