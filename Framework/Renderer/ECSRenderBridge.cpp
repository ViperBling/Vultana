#include "Renderer/ECSRenderBridge.hpp"
#include "AssetManager/SceneAssetRegistry.hpp"
#include "Core/VultanaEngine.hpp"
#include "RHI/RHI.hpp"

#include "Utilities/Log.hpp"

namespace Renderer
{
    FECSRenderBridge::FECSRenderBridge(FRendererBase* pRenderer, Assets::FSceneAssetRegistry* pAssetRegistry)
        : m_pRenderer(pRenderer)
        , m_pAssetRegistry(pAssetRegistry)
    {
    }

    uint32_t FECSRenderBridge::AddInstance(const FInstanceData& instanceData)
    {
        return m_pRenderer->AddInstance(instanceData);
    }

    void FECSRenderBridge::AddBasePassBatch(const Scene::ECS::FRenderBatchInput& input)
    {
        if (m_pAssetRegistry == nullptr) return;
        if (input.MeshletCount > 0)
        {
            RHI::FRHIPipelineState* pso = m_pAssetRegistry->GetMeshletPSO(input.MaterialHandle);
            if (pso == nullptr)
            {
                VTNA_LOG_WARN("[ECSRenderBridge] base pass batch '{}' skipped: no meshlet PSO for material handle {{{}, {}}}", input.Label, input.MaterialHandle.Index, input.MaterialHandle.Generation);
                return;
            }
            auto& batch = m_pRenderer->AddBasePassBatch();
            batch.Label = input.Label;
            batch.Center = input.Center;
            batch.Radius = input.Radius;
            batch.MeshletCount = input.MeshletCount;
            batch.InstanceIndex = input.InstanceIndex;
            batch.SetPipelineState(pso);
        }
        else
        {
            RHI::FRHIPipelineState* pso = m_pAssetRegistry->GetPSO(input.MaterialHandle);
            if (pso == nullptr) return;
            auto& batch = m_pRenderer->AddBasePassBatch();
            batch.Label = input.Label;
            batch.InstanceIndex = input.InstanceIndex;
            batch.SetPipelineState(pso);
            uint32_t rootConsts[1] = {input.InstanceIndex};
            batch.SetConstantBuffer(0, rootConsts, sizeof(rootConsts));
            batch.SetIndexBuffer(m_pRenderer->GetSceneStaticBuffer(), input.IndexBufferAddress, input.IndexStride == 2 ? RHI::ERHIFormat::R16UI : RHI::ERHIFormat::R32UI);
            batch.DrawIndexed(input.IndexCount);
        }
    }

    void FECSRenderBridge::AddObjectIDBatch(const Scene::ECS::FRenderBatchInput& input)
    {
        if (m_pAssetRegistry == nullptr) return;
        RHI::FRHIPipelineState* pso = m_pAssetRegistry->GetIDPSO(input.MaterialHandle);
        if (pso == nullptr) return;

        auto& batch = m_pRenderer->AddObjectIDPassBatch();
        batch.Label = input.Label;
        batch.InstanceIndex = input.InstanceIndex;
        batch.SetPipelineState(pso);

        uint32_t rootConsts[1] = {input.InstanceIndex};
        batch.SetConstantBuffer(0, rootConsts, sizeof(rootConsts));
        batch.SetIndexBuffer(m_pRenderer->GetSceneStaticBuffer(), input.IndexBufferAddress, input.IndexStride == 2 ? RHI::ERHIFormat::R16UI : RHI::ERHIFormat::R32UI);
        batch.DrawIndexed(input.IndexCount);
    }

    void FECSRenderBridge::AddOutlineBatch(const Scene::ECS::FRenderBatchInput& input)
    {
        if (m_pAssetRegistry == nullptr) return;
        RHI::FRHIPipelineState* pso = m_pAssetRegistry->GetOutlinePSO(input.MaterialHandle);
        if (pso == nullptr) return;

        auto& batch = m_pRenderer->AddOutlinePassBatch();
        batch.Label = input.Label;
        batch.InstanceIndex = input.InstanceIndex;
        batch.SetPipelineState(pso);

        uint32_t rootConsts[1] = {input.InstanceIndex};
        batch.SetConstantBuffer(0, rootConsts, sizeof(rootConsts));
        batch.SetIndexBuffer(m_pRenderer->GetSceneStaticBuffer(), input.IndexBufferAddress, input.IndexStride == 2 ? RHI::ERHIFormat::R16UI : RHI::ERHIFormat::R32UI);
        batch.DrawIndexed(input.IndexCount);
    }

    uint32_t FECSRenderBridge::AllocateMaterialConstants(const void* data, uint32_t size)
    {
        return m_pRenderer->AllocateSceneConstantBuffer(data, size);
    }

    uint32_t FECSRenderBridge::GetSelectedPickID() const
    {
        return m_pRenderer->GetMouseHitObjectID();
    }

    bool FECSRenderBridge::IsEnableMouseHitTest() const
    {
        return m_pRenderer->IsEnableMouseHitTest();
    }

    void FECSRenderBridge::AddAnimationBatch(const Scene::ECS::FAnimationBatchInput& input)
    {
        if (m_pAssetRegistry == nullptr || input.VertexCount == 0)
            return;
        RHI::FRHIPipelineState* pso = m_pAssetRegistry->GetSkinningPSO(input.MaterialHandle);
        if (pso == nullptr)
        {
            VTNA_LOG_WARN("[ECSRenderBridge] skinning batch '{}' skipped: no skinning PSO", input.Label);
            return;
        }
        auto& batch = m_pRenderer->AddAnimationBatch();
        batch.Label = input.Label;
        batch.SetPipelineState(pso);

        uint32_t cb[10] = {
            input.VertexCount,
            input.StaticPositionBufferAddress,
            input.StaticNormalBufferAddress,
            input.StaticTangentBufferAddress,
            input.AnimPositionBufferAddress,
            input.AnimNormalBufferAddress,
            input.AnimTangentBufferAddress,
            input.JointIDBufferAddress,
            input.JointWeightBufferAddress,
            input.JointMatricesAddress,
        };
        batch.SetConstantBuffer(1, cb, sizeof(cb));
        batch.Dispatch((input.VertexCount + 63) / 64, 1, 1);
    }
} // namespace Renderer