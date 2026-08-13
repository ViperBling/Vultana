#include "Scene/ECS/Processors/HierarchyProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

#include <EASTL/algorithm.h>
#include <EASTL/vector.h>
#include <EASTL/hash_map.h>

#include <cmath>
#include <algorithm>
#include <functional>

namespace Scene::ECS
{
    void FHierarchyProcessor::Process(FEntitySubsystem& subsystem, float deltaTime)
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
        ResetVisited();
        m_Stack.clear();

        auto rootView = registry.view<FTransformComponent>(entt::exclude_t<FParentComponent>());
        for (entt::entity root : rootView)
            m_Stack.push_back({root, float4x4{identity}});

        ProcessStack(registry, childrenMap);

        // 3. Detect and break cycles in the hierarchy
        m_ScratchDetach.clear();
        for (entt::entity seed : registry.view<FTransformComponent>())
        {
            if (IsVisited(seed))
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
                if (IsVisited(cur))
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

    void FHierarchyProcessor::ProcessStack(entt::registry& registry, const FEntitySubsystem::FChildrenMap& childrenMap)
    {
        while (!m_Stack.empty())
        {
            const FStackEntry entry = m_Stack.back();
            m_Stack.pop_back();

            if (IsVisited(entry.Entity))
                continue;
            SetVisited(entry.Entity);

            const auto* localTF = registry.try_get<FTransformComponent>(entry.Entity);
            float4x4 worldMtx = entry.ParentWorldMtx;

            if (localTF != nullptr)
            {
                const float4x4 T = translation_matrix(localTF->Position);
                const float4x4 R = rotation_matrix(quaternion(localTF->Rotation.x, localTF->Rotation.y, localTF->Rotation.z, localTF->Rotation.w));
                const float4x4 S = scaling_matrix(localTF->Scale);
                worldMtx = mul(entry.ParentWorldMtx, mul(T, mul(R, S)));

                FWorldTransformComponent& worldTF = registry.get<FWorldTransformComponent>(entry.Entity);
                worldTF.MtxWorld = worldMtx;
                worldTF.MtxWorldInvTranspose = transpose(inverse(worldMtx));

                if (const auto* localBounds = registry.try_get<FBoundingSphereComponent>(entry.Entity))
                {
                    FWorldBoundingSphereComponent& worldBounds = registry.get<FWorldBoundingSphereComponent>(entry.Entity);
                    worldBounds.Center = mul(worldMtx, float4(localBounds->Center, 1.0f)).xyz();
                    worldBounds.Radius = localBounds->Radius * MaxAxisScale(worldMtx);
                }

                if (const auto* localAABB = registry.try_get<FBoundingBoxComponent>(entry.Entity))
                {
                    FWorldBoundingBoxComponent& worldAABB = registry.get_or_emplace<FWorldBoundingBoxComponent>(entry.Entity);
                    const float3 center = (localAABB->Min + localAABB->Max) * 0.5f;
                    const float3 half = (localAABB->Max - localAABB->Min) * 0.5f;
                    const float3 worldCenter = mul(worldMtx, float4(center, 1.0f)).xyz();
                    // |M3x3| · half; translation column [3] stays out of the half-extents.
                    const float3 ax = abs(worldMtx[0].xyz());
                    const float3 ay = abs(worldMtx[1].xyz());
                    const float3 az = abs(worldMtx[2].xyz());
                    const float3 worldHalf = ax * half.x + ay * half.y + az * half.z;
                    worldAABB.Min = worldCenter - worldHalf;
                    worldAABB.Max = worldCenter + worldHalf;
                }
            }

            if (const auto it = childrenMap.find(entt::to_integral(entry.Entity)); it != childrenMap.end())
            {
                for (entt::entity child : it->second)
                    m_Stack.push_back({child, worldMtx});
            }
        }
    }

    bool FHierarchyProcessor::IsVisited(entt::entity e) const
    {
        const uint32_t idx = entt::to_entity(e);
        if (m_VisitedBits.size() <= (idx >> 6))
            return false;
        return (m_VisitedBits[idx >> 6] & (1ull << (idx & 63))) != 0;
    }

    void FHierarchyProcessor::SetVisited(entt::entity e)
    {
        const uint32_t idx = entt::to_entity(e);
        if (m_VisitedBits.size() <= (idx >> 6))
            m_VisitedBits.resize((idx >> 6) + 1, 0);
        m_VisitedBits[idx >> 6] |= 1ull << (idx & 63);
    }

    void FHierarchyProcessor::ResetVisited()
    {
        eastl::fill(m_VisitedBits.begin(), m_VisitedBits.end(), 0);
    }
}