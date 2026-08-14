#pragma once

#include "Scene/ECS/ProcessorBase.hpp"
#include "Scene/ECS/AnimationTypes.hpp"

namespace Scene::ECS
{
    class IAssetRegistry;

    class FAnimationProcessor final : public FProcessorBase
    {
    public:
        explicit FAnimationProcessor(const IAssetRegistry* registry) : m_Registry(registry) {}

        EProcessingPhase GetProcessingPhase() const override { return EProcessingPhase::PreUpdate; }
        const char* GetName() const override { return "AnimationProcessor"; }
        void Process(FEntitySubsystem& subsystem, float deltaTime) override;

    private:
        const IAssetRegistry* m_Registry = nullptr; // read-only external data source
    };
} // namespace Scene::ECS