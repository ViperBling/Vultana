#pragma once

#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/ProcessorBase.hpp"
#include "Utilities/Math.hpp"

#include <EASTL/vector.h>

#include <entt/entt.hpp>

namespace Scene::ECS
{
    // Processor that updates the hierarchy of entities based on their parent-child relationships.
    class FHierarchyProcessor final : public FProcessorBase
    {
    public:
        EProcessingPhase GetProcessingPhase() const override { return EProcessingPhase::PreUpdate; }
        const char* GetName() const override { return "HierarchyProcessor"; }
        void Process(FEntitySubsystem& subsystem, float deltaTime) override;

    private:
        struct FStackEntry
        {
            entt::entity Entity;
            float4x4 ParentWorldMtx;
        };

        void ProcessStack(entt::registry& registry, const FEntitySubsystem::FChildrenMap& childrenMap);
        bool IsVisited(entt::entity e) const;
        void SetVisited(entt::entity e);
        void ResetVisited();

    private:
        eastl::vector<FStackEntry> m_Stack;
        eastl::vector<uint64_t> m_VisitedBits;
        eastl::vector<entt::entity> m_ScratchDetach;
        eastl::vector<entt::entity> m_ScratchWalk;
    };
} // namespace Scene::ECS