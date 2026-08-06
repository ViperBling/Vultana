#pragma once

#include "Scene/ECS/RenderSink.hpp"
#include "Renderer/RendererBase.hpp"

namespace Assets
{
    class FSceneAssetRegistry;
}

namespace Renderer
{
    class FECSRenderBridge final : public Scene::ECS::ISceneRenderSink
    {
    public:
        explicit FECSRenderBridge(FRendererBase* pRenderer, Assets::FSceneAssetRegistry* pAssetRegistry);
        ~FECSRenderBridge() = default;

        uint32_t AddInstance(const FInstanceData& instanceData) override;
        void AddBasePassBatch(const Scene::ECS::FRenderBatchInput& input) override;
        void AddObjectIDBatch(const Scene::ECS::FRenderBatchInput& input) override;
        void AddOutlineBatch(const Scene::ECS::FRenderBatchInput& input) override;
        uint32_t AllocateMaterialConstants(const void* data, uint32_t size) override;
        uint32_t GetSelectedPickID() const override;
        bool IsEnableMouseHitTest() const override;
        void AddAnimationBatch(const Scene::ECS::FAnimationBatchInput& input) override;

    private:
        FRendererBase* m_pRenderer = nullptr;
        Assets::FSceneAssetRegistry* m_pAssetRegistry = nullptr;
    };
} // namespace Renderer