#pragma once

#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/ProcessorBase.hpp"
#include "Utilities/Math.hpp"

#include <EASTL/hash_set.h>
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

    private:
        eastl::vector<FStackEntry> m_Stack;
        eastl::hash_set<entt::id_type> m_VisitedEntities;
        eastl::vector<entt::entity> m_ScratchDetach;
        eastl::vector<entt::entity> m_ScratchWalk;
    };
} // namespace Scene::ECS