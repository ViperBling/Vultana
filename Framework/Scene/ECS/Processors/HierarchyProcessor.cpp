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

        const bool bStructureDirty = subsystem.ConsumeStructureDirty();
        const bool bTransformDirty = subsystem.ConsumeTransformDirty();
        if (!bStructureDirty && !bTransformDirty)
            return;

        // 1. Structural maintence only when the entity graph changed;
        if (bStructureDirty)
        {
            // a. Detach roots whose parents is null/invalid.
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

            // b. Cycle detection
            ResetVisited();
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
                else
                {
                    for (entt::entity w : m_ScratchWalk)        // acyclic chain → prune
                        SetVisited(w);
                }
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
            }

            // c. Hand a clean stamp to ProcessStack's dedup.
            ResetVisited();
        }

        // 2. Root set = entities with a transform and no parent.
        m_Stack.clear();
        if (bStructureDirty)
        {
            auto rootView = registry.view<FTransformComponent>(entt::exclude_t<FParentComponent>());
            for (entt::entity root : rootView)
                m_Stack.push_back({root, float4x4{identity}, true});
        }
        else
        {
            ResetVisited();
            for (entt::entity seed : subsystem.GetDirtyTransforms())
            {
                if (!registry.valid(seed) || IsVisited(seed))
                    continue;
                SetVisited(seed);
                if (HasDirtyAncestor(registry, seed))
                    continue;
                float4x4 parentWorld = float4x4{identity};
                if (const auto* parentComp = registry.try_get<FParentComponent>(seed))
                {
                    if (parentComp->Parent != entt::null && registry.valid(parentComp->Parent))
                    {
                        if (const auto* parentWorldComp = registry.try_get<FWorldTransformComponent>(parentComp->Parent))
                            parentWorld = parentWorldComp->MtxWorld;
                    }
                }
                m_Stack.push_back({seed, parentWorld, false});
            }
        }
        ProcessStack(registry, subsystem.GetChildrenMap(), bStructureDirty);
        subsystem.ClearDirtyTransforms();
    }

    bool FHierarchyProcessor::HasDirtyAncestor(const entt::registry& registry, entt::entity entity) const
    {
        constexpr uint32_t kMaxAncestorDepth = 4096;
        entt::entity cur = entity;
        for (uint32_t depth = 0; depth < kMaxAncestorDepth; ++depth)
        {
            const FParentComponent* parent = registry.try_get<FParentComponent>(cur);
            if (parent == nullptr || parent->Parent == entt::null || !registry.valid(parent->Parent))
                break;
            cur = parent->Parent;
            if (const auto* tf = registry.try_get<FTransformComponent>(cur))
            {
                if (tf->bLocalDirty)
                    return true;
            }
        }
        return false;
    }

    void FHierarchyProcessor::ProcessStack(entt::registry& registry, const FEntitySubsystem::FChildrenMap& childrenMap, bool bForceAll)
    {
        while (!m_Stack.empty())
        {
            const FStackEntry entry = m_Stack.back();
            m_Stack.pop_back();

            if (bForceAll)
            {
                if (IsVisited(entry.Entity))
                    continue;
                SetVisited(entry.Entity);
            }

            auto* localTF = registry.try_get<FTransformComponent>(entry.Entity);
            const bool bLocalDirty = (localTF != nullptr) && localTF->bLocalDirty;
            const bool bChanged = bForceAll || entry.bParentChanged || bLocalDirty;
            float4x4 worldMtx = entry.ParentWorldMtx;

            if (localTF != nullptr)
            {
                if (bChanged)
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
                        float4x4 boundsMtx = worldMtx;
                        if (const auto* skinned = registry.try_get<FSkinnedRootComponent>(entry.Entity))
                        {
                            const auto* rootWorld = registry.try_get<FWorldTransformComponent>(skinned->Root);
                            if (rootWorld != nullptr)
                                boundsMtx = rootWorld->MtxWorld;
                        }
                        worldBounds.Center = mul(boundsMtx, float4(localBounds->Center, 1.0f)).xyz();
                        worldBounds.Radius = localBounds->Radius * MaxAxisScale(boundsMtx);
                    }

                    if (const auto* localAABB = registry.try_get<FBoundingBoxComponent>(entry.Entity))
                    {
                        FWorldBoundingBoxComponent& worldAABB = registry.get_or_emplace<FWorldBoundingBoxComponent>(entry.Entity);
                        const float3 center = (localAABB->Min + localAABB->Max) * 0.5f;
                        const float3 half = (localAABB->Max - localAABB->Min) * 0.5f;
                        const float3 worldCenter = mul(worldMtx, float4(center, 1.0f)).xyz();
                        const float3 ax = abs(worldMtx[0].xyz());
                        const float3 ay = abs(worldMtx[1].xyz());
                        const float3 az = abs(worldMtx[2].xyz());
                        const float3 worldHalf = ax * half.x + ay * half.y + az * half.z;
                        worldAABB.Min = worldCenter - worldHalf;
                        worldAABB.Max = worldCenter + worldHalf;
                    }

                    if (bLocalDirty)
                        localTF->bLocalDirty = false;
                }
                else
                {
                    if (const auto* worldTF = registry.try_get<FWorldTransformComponent>(entry.Entity))
                        worldMtx = worldTF->MtxWorld;
                }
            }

            if (const auto it = childrenMap.find(entt::to_integral(entry.Entity)); it != childrenMap.end())
            {
                for (entt::entity child : it->second)
                    m_Stack.push_back({child, worldMtx, bChanged});
            }
        }
    }

    bool FHierarchyProcessor::IsVisited(entt::entity e) const
    {
        const uint32_t idx = entt::to_entity(e);
        const size_t word = idx >> 6;
        if (word >= m_VisitedBits.size() || m_VisitedStamp[word] != m_VisitedFrame)
            return false;
        return (m_VisitedBits[word] & (1ull << (idx & 63))) != 0;
    }

    void FHierarchyProcessor::SetVisited(entt::entity e)
    {
        const uint32_t idx = entt::to_entity(e);
        const size_t word = idx >> 6;
        if (m_VisitedBits.size() <= word)
        {
            m_VisitedBits.resize(word + 1, 0);
            m_VisitedStamp.resize(word + 1, 0);
        }
        if (m_VisitedStamp[word] != m_VisitedFrame)
        {
            m_VisitedStamp[word] = m_VisitedFrame;
            m_VisitedBits[word] = 0;
        }
        m_VisitedBits[word] |= (1ull << (idx & 63));
    }

    void FHierarchyProcessor::ResetVisited()
    {
        if (++m_VisitedFrame == 0) // wrap: rare, clear everything once
        {
            eastl::fill(m_VisitedStamp.begin(), m_VisitedStamp.end(), 0u);
            m_VisitedFrame = 1;
        }
    }
}