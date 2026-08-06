#include "Scene/ECS/Processors/RenderSubmissionProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include "Common/GPUScene.hlsli"
#include "Utilities/Math.hpp"

#include <EASTL/algorithm.h>

namespace Scene::ECS
{
    FRenderSubmissionProcessor::FRenderSubmissionProcessor(ISceneRenderSink& sink, const IAssetRegistry& registry)
        : m_Sink(sink), m_Registry(registry) {}

    void FRenderSubmissionProcessor::Process(FEntitySubsystem& subsystem, float deltaTime)
    {
        auto& registry = subsystem.GetRegistry();
        auto view = registry.view<FMeshRendererComponent, FWorldTransformComponent, FWorldBoundingSphereComponent>();
        const uint32_t selectedPickID = m_Sink.GetSelectedPickID();

        view.each([&](entt::entity entity, FMeshRendererComponent& meshComp, FWorldTransformComponent& worldTFComp, FWorldBoundingSphereComponent& worldBounds)
        {
            const auto& meshData = m_Registry.GetMeshData(meshComp.MeshHandle);
            if (!meshData.IsValid())
                return;
            
            FInstanceData instanceData = {};
            instanceData.IndexBufferAddress = meshData.IndexBufferAddress;
            instanceData.IndexStride = meshData.IndexStride;
            instanceData.TriangleCount = meshData.IndexCount / 3;
            instanceData.MeshletCount = meshData.MeshletCount;
            instanceData.MeshletBufferAddress = meshData.MeshletBufferAddress;
            instanceData.MeshletVertexBufferAddress = meshData.MeshletVertexBufferAddress;
            instanceData.MeshletIndexBufferAddress = meshData.MeshletIndexBufferAddress;
            instanceData.PositionBufferAddress = meshData.PositionBufferAddress;
            instanceData.TexCoordBufferAddress = meshData.TexCoordBufferAddress;
            instanceData.NormalBufferAddress = meshData.NormalBufferAddress;
            instanceData.TangentBufferAddress = meshData.TangentBufferAddress;
            instanceData.bVertexAnimation = meshComp.bSkinned ? 1 : 0;
            instanceData.ObjectID = subsystem.GetPickID(entity);
            instanceData.Scale = eastl::max(eastl::max(Abs(worldTFComp.MtxWorld[0].x), Abs(worldTFComp.MtxWorld[1].y)), Abs(worldTFComp.MtxWorld[2].z));
            instanceData.Center = worldBounds.Center;
            instanceData.Radius = worldBounds.Radius;
            instanceData.MtxWorld = worldTFComp.MtxWorld;
            instanceData.MtxWorldInvTranspose = worldTFComp.MtxWorldInvTranspose;

            auto* visibilityComp = registry.try_get<FVisibilityComponent>(entity);
            const bool bVisible = visibilityComp && visibilityComp->bFrustumVisible;

            // Only allocate material constants for instances that will be drawn this frame.
            // Invisible instances are still submitted to the GPU scene for GPU-driven culling,
            // but are never batched, so their MaterialDataAddress is never read.
            if (bVisible)
            {
                const auto& materialData = m_Registry.GetMaterialData(meshComp.MaterialHandle);
                instanceData.MaterialDataAddress = m_Sink.AllocateMaterialConstants(materialData.Data, sizeof(materialData.Data));
            }

            uint32_t instanceIndex = m_Sink.AddInstance(instanceData);

            if (bVisible)
            {
                FRenderBatchInput bin;
                bin.InstanceIndex = instanceIndex;
                bin.MeshletCount = meshData.MeshletCount;
                bin.Center = worldBounds.Center;
                bin.Radius = worldBounds.Radius;
                bin.MaterialHandle = meshComp.MaterialHandle;
                bin.IndexBufferAddress = meshData.IndexBufferAddress;
                bin.IndexStride = meshData.IndexStride;
                bin.IndexCount = meshData.IndexCount;
                bin.Label = "ECS_Mesh";
                m_Sink.AddBasePassBatch(bin);
                
                if (m_Sink.IsEnableMouseHitTest()) m_Sink.AddObjectIDBatch(bin);
                if (instanceData.ObjectID == selectedPickID) m_Sink.AddOutlineBatch(bin);
            }
        });
    }
}