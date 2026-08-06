#pragma once

#include "Scene/ECS/ProcessorBase.hpp"
#include "Scene/ECS/RenderSink.hpp"
#include "Scene/ECS/AssetRegistry.hpp"

namespace Scene::ECS
{
    class FRenderSubmissionProcessor final : public FProcessorBase
    {
    public:
        FRenderSubmissionProcessor(ISceneRenderSink& sink, const IAssetRegistry& registry);
        EProcessingPhase GetProcessingPhase() const override { return EProcessingPhase::Render; }
        const char* GetName() const override { return "RenderSubmissionProcessor"; }
        void Process(FEntitySubsystem& subsystem, float deltaTime) override;

    private:
        ISceneRenderSink& m_Sink;
        const IAssetRegistry& m_Registry;
    };
} // namespace Scene::ECS