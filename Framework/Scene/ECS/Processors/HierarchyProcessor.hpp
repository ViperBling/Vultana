#pragma once

#include "Scene/ECS/ProcessorBase.hpp"

namespace Scene::ECS
{
    // Processor that updates the hierarchy of entities based on their parent-child relationships.
    class FHierarchyProcessor final : public FProcessorBase
    {
    public:
        EProcessingPhase GetProcessingPhase() const override { return EProcessingPhase::PreUpdate; }
        const char* GetName() const override { return "HierarchyProcessor"; }
        void Process(FEntitySubsystem& subsystem, float deltaTime) override;
    };
} // namespace Scene::ECS