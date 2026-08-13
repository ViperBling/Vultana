#include "Scene/ECS/Processors/HierarchyProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

#include <EASTL/algorithm.h>
#include <EASTL/vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>

#include <cmath>
#include <algorithm>
#include <functional>

namespace Scene::ECS
{
    void Scene::ECS::FHierarchyProcessor::Process(FEntitySubsystem& subsystem, float deltaTime)
    {
        auto& registry = subsystem.GetRegistry();

        // 1. Build adjacency list of parent-child relationships
        m_ScratchDetach.clear();
        auto parentView = registry.view<FParentComponent>();
        for (entt::entity entity : parentView)
        {
            const FParentComponent& parentComp = parentView.get<FParentComponent>(entity);
            if (parentComp.Parent == entt::null || !registry.valid(parentComp.Parent))
                m_ScratchDetach.push_back(entity);
        }
        for (entt::entity entity : m_ScratchDetach)
            subsystem.DetachFromParent(entity);

        // 2. Process the hierarchy starting from root entities
        const FEntitySubsystem::FChildrenMap& childrenMap = subsystem.GetChildrenMap();
        m_VisitedEntities.clear();
        m_Stack.clear();

        auto rootView = registry.view<FTransformComponent>(entt::exclude_t<FParentComponent>());
        for (entt::entity root : rootView)
            m_Stack.push_back({root, float4x4{identity}});

        ProcessStack(registry, childrenMap);

        // 3. Detect and break cycles in the hierarchy
        m_ScratchDetach.clear();
        for (entt::entity seed : registry.view<FTransformComponent>())
        {
            if (m_VisitedEntities.find(entt::to_integral(seed)) != m_VisitedEntities.end())
                continue;

            m_ScratchWalk.clear();
            m_ScratchWalk.push_back(seed);
            entt::entity cur = seed;
            bool bInCycle = false;
            for (uint32_t depth = 0; depth < 4096; ++depth)
            {
                const auto* parentComp = registry.try_get<FParentComponent>(cur);
                if (parentComp == nullptr || parentComp->Parent == entt::null || !registry.valid(parentComp->Parent))
                    break;
                cur = parentComp->Parent;
                if (m_VisitedEntities.find(entt::to_integral(cur)) != m_VisitedEntities.end())
                    break;
                if (eastl::find(m_ScratchWalk.begin(), m_ScratchWalk.end(), cur) != m_ScratchWalk.end())
                {
                    bInCycle = true;
                    break;
                }
                m_ScratchWalk.push_back(cur);
            }
            if (bInCycle)
                m_ScratchDetach.push_back(cur);
        }
        if (!m_ScratchDetach.empty())
        {
            entt::entity cycleRoot = m_ScratchDetach[0];
            for (entt::entity e : m_ScratchDetach)
            {
                if (entt::to_integral(e) < entt::to_integral(cycleRoot))
                    cycleRoot = e;
            }
            VTNA_LOG_WARN("HierarchyProcessor: cycle detected ({} member(s) on cycle), breaking at entity {}", m_ScratchDetach.size(), entt::to_integral(cycleRoot));

            subsystem.DetachFromParent(cycleRoot);

            m_Stack.clear();
            m_Stack.push_back({cycleRoot, float4x4{identity}});
            ProcessStack(registry, childrenMap);
        }
    }

    void Scene::ECS::FHierarchyProcessor::ProcessStack(entt::registry& registry, const FEntitySubsystem::FChildrenMap& childrenMap)
    {
        while (!m_Stack.empty())
        {
            const FStackEntry entry = m_Stack.back();
            m_Stack.pop_back();

            if (!m_VisitedEntities.insert(entt::to_integral(entry.Entity)).second)
                continue;

            const auto* localTF = registry.try_get<FTransformComponent>(entry.Entity);
            float4x4 worldMtx = entry.ParentWorldMtx;

            if (localTF != nullptr)
            {
                const float4x4 T = translation_matrix(localTF->Position);
                const float4x4 R = rotation_matrix(quaternion(localTF->Rotation.x, localTF->Rotation.y, localTF->Rotation.z, localTF->Rotation.w));
                const float4x4 S = scaling_matrix(localTF->Scale);
                worldMtx = mul(entry.ParentWorldMtx, mul(T, mul(R, S)));

                FWorldTransformComponent& worldTF = registry.get_or_emplace<FWorldTransformComponent>(entry.Entity);
                worldTF.MtxWorld = worldMtx;
                worldTF.MtxWorldInvTranspose = transpose(inverse(worldMtx));

                if (const auto* localBounds = registry.try_get<FBoundingSphereComponent>(entry.Entity))
                {
                    FWorldBoundingSphereComponent& worldBounds = registry.get_or_emplace<FWorldBoundingSphereComponent>(entry.Entity);
                    worldBounds.Center = mul(worldMtx, float4(localBounds->Center, 1.0f)).xyz();
                    worldBounds.Radius = localBounds->Radius * MaxAxisScale(worldMtx);
                }
            }

            if (const auto it = childrenMap.find(entt::to_integral(entry.Entity)); it != childrenMap.end())
            {
                for (entt::entity child : it->second)
                    m_Stack.push_back({child, worldMtx});
            }
        }
    }
}