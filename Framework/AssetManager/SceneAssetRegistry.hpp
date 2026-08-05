#pragma once

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
        void FrameFencePassed(uint64_t frameID) override;

        // PSO resolution (RHI layer, called by ECSRenderBridge; Core interface remains RHI-free)
        RHI::FRHIPipelineState* GetMeshletPSO(Scene::ECS::FMaterialResourceHandle handle) const;
        RHI::FRHIPipelineState* GetIDPSO(Scene::ECS::FMaterialResourceHandle handle) const;
        RHI::FRHIPipelineState* GetOutlinePSO(Scene::ECS::FMaterialResourceHandle handle) const;

    private:
        struct FMeshRecord
        {
            uint32_t Generation  = 0;
            int32_t RefCount     = 0;
            Scene::ECS::FMeshGPUData GPUData;
            bool bPendingRelease = false;
        };

        struct FMaterialRecord
        {
            uint32_t Generation  = 0;
            int32_t RefCount     = 0;
            Scene::ECS::FMaterialGPUData GPUData;
            Assets::FMeshMaterial *Material = nullptr; // Fill by ModelLoader
            bool bPendingRelease = false;
        };

        struct FModelAsset
        {
            uint32_t Generation     = 0;
            eastl::string ModelPath;
            eastl::vector<Scene::ECS::FModelNodeInfo> Nodes;
            uint32_t FirstMesh      = 0;
            uint32_t MeshCount      = 0;
            uint32_t FirstMaterial  = 0;
            uint32_t MaterialCount  = 0;
            uint32_t PrimitiveCount = 0;
        };

        struct FPendingRelease
        {
            enum class EType
            {
                Mesh,
                Material
            };
            EType Type;
            uint32_t RecordIndex;
        };

        Renderer::FRendererBase* m_pRenderer = nullptr;
        eastl::vector<FModelAsset> m_Assets;
        eastl::vector<FMeshRecord> m_Meshes;
        eastl::vector<FMaterialRecord> m_Materials;
        eastl::vector<FPendingRelease> m_PendingReleaseQueue;
    };
}