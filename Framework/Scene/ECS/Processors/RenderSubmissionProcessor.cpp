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
            const FMeshGPUData* meshData = m_Registry.FindMeshData(meshComp.MeshHandle);
            if (meshData == nullptr || !meshData->IsValid())
                return;
            
            FInstanceData instanceData = {};
            instanceData.MaterialDataAddress = NULL_CONSTANT_ADDRESS;
            instanceData.IndexBufferAddress = meshData->IndexBufferAddress;
            instanceData.IndexStride = meshData->IndexStride;
            instanceData.TriangleCount = meshData->IndexCount / 3;
            instanceData.MeshletCount = meshData->MeshletCount;
            instanceData.MeshletBufferAddress = meshData->MeshletBufferAddress;
            instanceData.MeshletVertexBufferAddress = meshData->MeshletVertexBufferAddress;
            instanceData.MeshletIndexBufferAddress = meshData->MeshletIndexBufferAddress;
            instanceData.PositionBufferAddress = meshData->PositionBufferAddress;
            instanceData.TexCoordBufferAddress = meshData->TexCoordBufferAddress;
            instanceData.NormalBufferAddress = meshData->NormalBufferAddress;
            instanceData.TangentBufferAddress = meshData->TangentBufferAddress;
            instanceData.bVertexAnimation = meshComp.bSkinned ? 1 : 0;
            instanceData.ObjectID = registry.get<FPickIDComponent>(entity).PickID;
            instanceData.Scale = MaxAxisScale(worldTFComp.MtxWorld);
            instanceData.Center = worldBounds.Center;
            instanceData.Radius = worldBounds.Radius;
            instanceData.MtxWorld = worldTFComp.MtxWorld;
            instanceData.MtxWorldInvTranspose = worldTFComp.MtxWorldInvTranspose;

            const bool bVisible = registry.get<FVisibilityComponent>(entity).bFrustumVisible;
            if (bVisible)
            {
                if (const FMaterialGPUData* materialData = m_Registry.FindMaterialData(meshComp.MaterialHandle))
                {
                    const uint32_t addr = m_Sink.AllocateMaterialConstants(materialData->Data, sizeof(materialData->Data));
                    if (addr != NULL_CONSTANT_ADDRESS)
                    {
                        instanceData.MaterialDataAddress = addr;
                    }
                }
            }

            uint32_t instanceIndex = m_Sink.AddInstance(instanceData);

            if (bVisible)
            {
                FRenderBatchInput bin;
                bin.InstanceIndex = instanceIndex;
                bin.MeshletCount = meshData->MeshletCount;
                bin.Center = worldBounds.Center;
                bin.Radius = worldBounds.Radius;
                bin.MaterialHandle = meshComp.MaterialHandle;
                bin.IndexBufferAddress = meshData->IndexBufferAddress;
                bin.IndexStride = meshData->IndexStride;
                bin.IndexCount = meshData->IndexCount;
                bin.Label = "ECS_Mesh";
                m_Sink.AddBasePassBatch(bin);
                
                if (m_Sink.IsEnableMouseHitTest()) m_Sink.AddObjectIDBatch(bin);
                if (instanceData.ObjectID == selectedPickID) m_Sink.AddOutlineBatch(bin);
            } });
    }
}