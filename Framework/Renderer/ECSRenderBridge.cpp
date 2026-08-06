#include "ECSRenderBridge.hpp"
#include "AssetManager/SceneAssetRegistry.hpp"
#include "RHI/RHI.hpp"
#include "Core/VultanaEngine.hpp"

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
        auto& batch = m_pRenderer->AddBasePassBatch();
        batch.Label = input.Label;
        batch.Center = input.Center;
        batch.Radius = input.Radius;
        batch.MeshletCount = input.MeshletCount;
        batch.InstanceIndex = input.InstanceIndex;
        if (m_pAssetRegistry)
        {
            if (RHI::FRHIPipelineState* pso = m_pAssetRegistry->GetMeshletPSO(input.MaterialHandle))
            {
                batch.SetPipelineState(pso);
            }
        }
    }

    void FECSRenderBridge::AddObjectIDBatch(const Scene::ECS::FRenderBatchInput& input)
    {
        auto& batch = m_pRenderer->AddObjectIDPassBatch();
        batch.Label = input.Label;
        batch.InstanceIndex = input.InstanceIndex;

        if (!m_pAssetRegistry) return;
        RHI::FRHIPipelineState* pso = m_pAssetRegistry->GetIDPSO(input.MaterialHandle);
        if (!pso) return;

        batch.SetPipelineState(pso);
        uint32_t rootConsts[1] = {input.InstanceIndex};
        batch.SetConstantBuffer(0, rootConsts, sizeof(rootConsts));
        batch.SetIndexBuffer(m_pRenderer->GetSceneStaticBuffer(), input.IndexBufferAddress, input.IndexStride == 2 ? RHI::ERHIFormat::R16UI : RHI::ERHIFormat::R32UI);
        batch.DrawIndexed(input.IndexCount);
    }

    void FECSRenderBridge::AddOutlineBatch(const Scene::ECS::FRenderBatchInput& input)
    {
        auto& batch = m_pRenderer->AddOutlinePassBatch();
        batch.Label = input.Label;
        batch.InstanceIndex = input.InstanceIndex;

        if (!m_pAssetRegistry) return;
        RHI::FRHIPipelineState* pso = m_pAssetRegistry->GetOutlinePSO(input.MaterialHandle);
        if (!pso) return;

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
    }
} // namespace Renderer