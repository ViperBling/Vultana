#pragma once

#include "Scene/ECS/ProcessorBase.hpp"
#include "Scene/ECS/SceneLightSnapshot.hpp"

namespace Scene::ECS
{
    class FLightProcessor final : public FProcessorBase
    {
    public:
        explicit FLightProcessor(FSceneLightSnapshot* output) : m_pOutput(output) {}

        EProcessingPhase GetProcessingPhase() const override { return EProcessingPhase::PreUpdate; }
        const char* GetName() const override { return "LightProcessor"; }
        void Process(FEntitySubsystem& subsystem, float deltaTime) override;

        const FSceneLightSnapshot* GetOutput() const { return m_pOutput; }

    private:
        FSceneLightSnapshot* m_pOutput = nullptr;
    };
} // namespace Scene::ECS