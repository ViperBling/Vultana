#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "Scene/ECS/AssetRegistry.hpp"
#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>

namespace Assets
{
    class FMeshMaterial;

    class FSceneAssetRegistry final : public Scene::ECS::IAssetRegistry
    {
    public:
        explicit FSceneAssetRegistry(Renderer::FRendererBase *pRenderer);
        ~FSceneAssetRegistry();

        Scene::ECS::FModelAssetHandle ImportModel(const eastl::string& assetFile) override;
        Scene::ECS::FMeshResourceHandle AcquireMesh(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx) override;
        Scene::ECS::FMaterialResourceHandle AcquireMaterial(Scene::ECS::FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx) override;
        void Release(Scene::ECS::FMeshResourceHandle handle) override;
        void Release(Scene::ECS::FMaterialResourceHandle handle) override;
        Scene::ECS::FMeshGPUData GetMeshData(Scene::ECS::FMeshResourceHandle handle) const override;
        Scene::ECS::FMaterialGPUData GetMaterialData(Scene::ECS::FMaterialResourceHandle handle) const override;
        Scene::ECS::FModelAssetInfo GetModelInfo(Scene::ECS::FModelAssetHandle handle) const override;
        Scene::ECS::EAssetState GetState(Scene::ECS::FModelAssetHandle handle) const override;
        void FrameFencePassed(uint64_t frameID) override;
        void DeferDeleteMaterial(FMeshMaterial* material);

        // PSO resolution (RHI layer, called by ECSRenderBridge; Core interface remains RHI-free)
        RHI::FRHIPipelineState* GetMeshletPSO(Scene::ECS::FMaterialResourceHandle handle) const;
        RHI::FRHIPipelineState* GetIDPSO(Scene::ECS::FMaterialResourceHandle handle) const;
        RHI::FRHIPipelineState* GetOutlinePSO(Scene::ECS::FMaterialResourceHandle handle) const;

        void PopulateModel(Scene::ECS::FModelAssetHandle asset, const FModelCookedData& model); 
        void MarkModelFailed(Scene::ECS::FModelAssetHandle asset);

        bool ReloadModel(Scene::ECS::FModelAssetHandle asset, const FModelCookedData& model);

        struct FModelStatus
        {
            eastl::string ModelPath;
            Scene::ECS::EAssetState State = Scene::ECS::EAssetState::Pending;
            uint32_t PrimitiveCount = 0;
            uint32_t MeshCount = 0;
            uint32_t MaterialCount = 0;
            uint32_t RefCount = 0;
            uint32_t ReloadCount = 0;
        };

        void EnumerateModels(eastl::vector<FModelStatus>& out) const;
        bool FindModelHandle(const eastl::string& modelPath, Scene::ECS::FModelAssetHandle& outHandle) const;

        void ReloadTexture(const eastl::string& productPath);
        struct FTextureStatus
        {
            eastl::string ProductPath;
            bool bResident = false;
        };
        void EnumerateTextures(eastl::vector<FTextureStatus>& out) const;

    private:
        struct FMeshRecord
        {
            uint32_t Generation  = 0;
            int32_t RefCount     = 0;
            Scene::ECS::FMeshGPUData GPUData;
            eastl::vector<OffsetAllocator::Allocation> Allocations;
            bool bPendingRelease = false;
        };

        struct FMaterialRecord
        {
            uint32_t Generation  = 0;
            int32_t RefCount     = 0;
            Scene::ECS::FMaterialGPUData GPUData;
            Assets::FMeshMaterial* Material = nullptr;
            bool bPendingRelease = false;
            int32_t AssetMaterialIndex = -1;
            bool bSkeletalAnim = false;
            bool bFrontFaceCCW = false;
        };

        struct FModelAsset
        {
            uint32_t Generation = 0;
            uint32_t ReloadCount = 0;
            eastl::string ModelPath;
            Scene::ECS::EAssetState State = Scene::ECS::EAssetState::Pending;
            eastl::vector<Scene::ECS::FModelNodeInfo> Nodes;
            eastl::vector<Scene::ECS::FMeshResourceHandle> MeshHandles;
            eastl::vector<Scene::ECS::FMaterialResourceHandle> MaterialHandles;
            uint32_t PrimitiveCount = 0;
            uint32_t FirstMesh = UINT32_MAX;
            uint32_t MeshCount = 0;
            uint32_t FirstMaterial = UINT32_MAX;
            uint32_t MaterialCount = 0;
            eastl::vector<FAssetMaterial> Materials;
            eastl::vector<eastl::string> TextureRefs;
        };

        struct FPendingRelease
        {
            enum class EType
            {
                Mesh,
                Material,
                MaterialObject
            };
            EType Type;
            uint32_t RecordIndex;
            FMeshMaterial* MaterialPtr = nullptr;
            uint64_t TargetFrame = 0;
        };

        Renderer::FRendererBase* m_pRenderer = nullptr;
        eastl::vector<FModelAsset> m_Assets;
        eastl::vector<FMeshRecord> m_Meshes;
        eastl::vector<FMaterialRecord> m_Materials;
        eastl::vector<FPendingRelease> m_PendingReleaseQueue;
    };
}