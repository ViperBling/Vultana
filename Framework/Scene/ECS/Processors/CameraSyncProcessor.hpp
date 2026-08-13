#pragma once

#include "Scene/ECS/ProcessorBase.hpp"
#include "Scene/ECS/SceneCameraSnapshot.hpp"

namespace Scene::ECS
{
    class FCameraSyncProcessor final : public FProcessorBase
    {
    public:
        explicit FCameraSyncProcessor(FSceneCameraSnapshot* output) : m_pOutput(output) {}

        EProcessingPhase GetProcessingPhase() const override { return EProcessingPhase::PreUpdate; }
        const char* GetName() const override { return "CameraSyncProcessor"; }
        void Process(FEntitySubsystem& subsystem, float deltaTime) override;

        const FSceneCameraSnapshot* GetOutput() const { return m_pOutput; }

    private:
        FSceneCameraSnapshot* m_pOutput = nullptr;
    };
} // namespace Scene::ECS