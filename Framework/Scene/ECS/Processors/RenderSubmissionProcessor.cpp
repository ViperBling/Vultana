#include "Scene/ECS/Processors/RenderSubmissionProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include "Common/GPUScene.hlsli"

#include "Utilities/Log.hpp"
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
            if (meshData == nullptr)
                return;
            if (!meshData->IsValid() && !(meshComp.bSkinned && meshData->IndexCount > 0))
                return;
            const FSkinnedRootComponent* skinnedRoot = registry.try_get<FSkinnedRootComponent>(entity);
            float4x4 instanceWorld = worldTFComp.MtxWorld; // static: node-anchored world
            float3 boundsCenter = worldBounds.Center;
            float boundsRadius = worldBounds.Radius;
            if (skinnedRoot != nullptr)
            {
                const FWorldTransformComponent* rootWorld = registry.try_get<FWorldTransformComponent>(skinnedRoot->Root);
                if (rootWorld == nullptr)
                {
                    static bool bWarnedMissingRootWorld = false;
                    if (!bWarnedMissingRootWorld)
                    {
                        bWarnedMissingRootWorld = true;
                        VTNA_LOG_WARN("RenderSubmission: skinned mesh {} has no root world transform, skipped", entt::to_integral(entity));
                    }
                    return;
                }
                instanceWorld = rootWorld->MtxWorld;
                boundsCenter = mul(instanceWorld, float4(meshData->Center, 1.0f)).xyz();
                boundsRadius = meshData->Radius * MaxAxisScale(instanceWorld);
            }

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
            instanceData.bVertexAnimation = (meshComp.bSkinned && meshData->AnimPositionBufferAddress != Scene::ECS::INVALID_ADDRESS) ? 1 : 0;
            instanceData.ObjectID = registry.get<FPickIDComponent>(entity).PickID;
            if (meshComp.bSkinned)
            {
                if (meshData->AnimPositionBufferAddress != Scene::ECS::INVALID_ADDRESS)
                    instanceData.PositionBufferAddress = meshData->AnimPositionBufferAddress;
                if (meshData->AnimNormalBufferAddress != Scene::ECS::INVALID_ADDRESS)
                    instanceData.NormalBufferAddress = meshData->AnimNormalBufferAddress;
                if (meshData->AnimTangentBufferAddress != Scene::ECS::INVALID_ADDRESS)
                    instanceData.TangentBufferAddress = meshData->AnimTangentBufferAddress;
            }
            instanceData.Scale = MaxAxisScale(instanceWorld);
            instanceData.Center = boundsCenter;
            instanceData.Radius = boundsRadius;
            instanceData.MtxWorld = instanceWorld;
            instanceData.MtxWorldInvTranspose = (skinnedRoot != nullptr) ? transpose(inverse(instanceWorld)) : worldTFComp.MtxWorldInvTranspose;

            const bool bVisible = registry.get<FVisibilityComponent>(entity).bFrustumVisible;
            instanceData.MaterialDataAddress = m_Registry.GetMaterialConstantAddress(meshComp.MaterialHandle);
            if (instanceData.MaterialDataAddress == Scene::ECS::INVALID_ADDRESS)
                instanceData.MaterialDataAddress = NULL_CONSTANT_ADDRESS;

            uint32_t instanceIndex = m_Sink.AddInstance(instanceData);

            if (bVisible)
            {
                if (meshComp.bSkinned && skinnedRoot != nullptr)
                {
                    const FModelInstanceComponent* modelComp = registry.try_get<FModelInstanceComponent>(skinnedRoot->Root);
                    const FSkeletonInfo* skeleton = (modelComp != nullptr) ? m_Registry.GetSkeletonInfo(modelComp->ModelHandle) : nullptr;
                    const FModelNodeTableComponent* nodeTable = registry.try_get<FModelNodeTableComponent>(skinnedRoot->Root);
                    if (modelComp != nullptr && skeleton != nullptr && nodeTable != nullptr && !skeleton->JointNodeIndices.empty() &&  skeleton->JointNodeIndices.size() == skeleton->InverseBindMatrices.size())
                    {
                        m_ScratchJointMatrices.clear();
                        m_ScratchJointMatrices.resize(skeleton->JointNodeIndices.size());
                        const float4x4 rootInv = inverse(instanceWorld);
                        for (size_t j = 0; j < m_ScratchJointMatrices.size(); ++j)
                        {
                            float4x4 m = skeleton->InverseBindMatrices[j];
                            const uint32_t nodeIdx = static_cast<uint32_t>(skeleton->JointNodeIndices[j]);
                            if (nodeIdx < nodeTable->NodeEntities.size())
                            {
                                const entt::entity nodeEntity = nodeTable->NodeEntities[nodeIdx];
                                if (nodeEntity != entt::null && registry.valid(nodeEntity))
                                    if (const auto* nodeWorld = registry.try_get<FWorldTransformComponent>(nodeEntity))
                                        m = mul(mul(rootInv, nodeWorld->MtxWorld), m); // model-space (D2)
                            }
                            m_ScratchJointMatrices[j] = m;
                        }
                        const uint32_t jointAddr = m_Sink.AllocateMaterialConstants(
                            m_ScratchJointMatrices.data(), static_cast<uint32_t>(m_ScratchJointMatrices.size() * sizeof(float4x4)));
                        if (jointAddr == NULL_CONSTANT_ADDRESS)
                            return;

                        FAnimationBatchInput bin;
                        bin.MaterialHandle = meshComp.MaterialHandle;
                        bin.VertexCount = meshData->VertexCount;
                        bin.StaticPositionBufferAddress = meshData->PositionBufferAddress;
                        bin.AnimPositionBufferAddress = meshData->AnimPositionBufferAddress;
                        bin.AnimNormalBufferAddress = meshData->AnimNormalBufferAddress;
                        bin.AnimTangentBufferAddress = meshData->AnimTangentBufferAddress;
                        bin.StaticNormalBufferAddress = meshData->bHasNormalStream ? meshData->NormalBufferAddress : 0xFFFFFFFFu;
                        bin.StaticTangentBufferAddress = meshData->bHasTangentStream ? meshData->TangentBufferAddress : 0xFFFFFFFFu;
                        bin.JointIDBufferAddress = meshData->JointIDBufferAddress;
                        bin.JointWeightBufferAddress = meshData->JointWeightBufferAddress;
                        bin.JointMatricesAddress = jointAddr;
                        bin.Label = "ECS_Skin";
                        m_Sink.AddAnimationBatch(bin);
                    }
                }

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
            } 
        });
    }
}