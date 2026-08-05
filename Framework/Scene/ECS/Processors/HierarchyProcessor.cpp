#include "Scene/ECS/Processors/HierarchyProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Fragments.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

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
        struct FChildList
        {
            eastl::vector<entt::entity> Children;
        };
        eastl::hash_map<entt::id_type, FChildList> adjacencyList;
        auto parentView = registry.view<FParentFragment>();
        for (auto entity : parentView)
        {
            auto& parentFrag = parentView.get<FParentFragment>(entity);
            entt::entity parent = parentFrag.Parent;
            if (parent == entt::null) continue; // Skip if no parent
            if (!registry.valid(parent))
            {
                registry.remove<FParentFragment>(entity);
                continue; // Skip if parent is invalid
            }
            adjacencyList[entt::to_integral(parent)].Children.push_back(entity);
        }

        // 2. Detect cycles in the hierarchy using DFS and break them if found
        using VisitState = eastl::hash_map<entt::id_type, uint8_t>;
        VisitState visitState;
        auto rootView = registry.view<FTransformFragment>(entt::exclude_t<FParentFragment>());

        std::function<bool(entt::entity, VisitState&, entt::entity&)> detectCycle;
        detectCycle = [&](entt::entity entity, VisitState& state, entt::entity& cycleRoot) -> bool
        {
            auto id = entt::to_integral(entity);
            auto it = state.find(id);
            if (it != state.end())
            {
                if (it->second == 1)
                {
                    cycleRoot = entity;
                    return true;
                }
                return false;
            }
            state[id] = 1;
            auto adjIt = adjacencyList.find(id);
            if (adjIt != adjacencyList.end())
                for (entt::entity child : adjIt->second.Children)
                    if (detectCycle(child, state, cycleRoot))
                        return true;
            state[id] = 2;
            return false;
        };

        for (entt::entity root : rootView)
        {
            entt::entity cycleRoot = entt::null;
            if (detectCycle(root, visitState, cycleRoot))
            {
                VTNA_LOG_WARN("HierarchyProcessor: cycle detected near entity {}, breaking", entt::to_integral(cycleRoot));
                eastl::vector<entt::entity> cycleEntities;
                for (auto en : registry.view<FParentFragment>())
                {
                    auto id = entt::to_integral(en);
                    auto it = visitState.find(id);
                    if (it != visitState.end() && it->second == 1)
                        cycleEntities.push_back(en);
                }
                if (!cycleEntities.empty())
                {
                    auto minIt = std::min_element(cycleEntities.begin(), cycleEntities.end(),
                        [](entt::entity a, entt::entity b)
                        { return entt::to_integral(a) < entt::to_integral(b); });
                    for (entt::entity e : cycleEntities)
                    {
                        if (e != *minIt)
                            registry.remove<FParentFragment>(e);
                    }
                }
            }
        }

        // 3. DFS traversal to update MtxWorld and WorldBounds
        std::function<void(entt::entity, const float4x4&)> processEntity;
        processEntity = [&](entt::entity entity, const float4x4& parentWorld)
        {
            auto* localTF = registry.try_get<FTransformFragment>(entity);
            if (!localTF)
            {
                auto adjIt = adjacencyList.find(entt::to_integral(entity));
                if (adjIt != adjacencyList.end())
                    for (entt::entity child : adjIt->second.Children)
                        processEntity(child, parentWorld);
                return;
            }
            float4x4 T = translation_matrix(localTF->Position);
            float4x4 R = rotation_matrix(quaternion(localTF->Rotation.x, localTF->Rotation.y, localTF->Rotation.z, localTF->Rotation.w));
            float4x4 S = scaling_matrix(localTF->Scale);
            float4x4 localMtx = mul(T, mul(R, S));
            float4x4 worldMtx = mul(parentWorld, localMtx);

            registry.emplace_or_replace<FWorldTransformFragment>(entity);
            auto* worldTF = registry.try_get<FWorldTransformFragment>(entity);
            if (worldTF)
            {
                worldTF->MtxWorld = worldMtx;
                worldTF->MtxWorldInvTranspose = transpose(inverse(worldMtx));
            }

            auto* localBounds = registry.try_get<FBoundingSphereFragment>(entity);
            if (localBounds && worldTF)
            {
                registry.emplace_or_replace<FWorldBoundingSphereFragment>(entity);
                auto *worldBounds = registry.try_get<FWorldBoundingSphereFragment>(entity);
                if (worldBounds)
                {
                    float4 centerWS = mul(worldMtx, float4(localBounds->Center, 1.0f));
                    worldBounds->Center = centerWS.xyz();
                    float maxScale = eastl::max(eastl::max(Abs(localTF->Scale.x), Abs(localTF->Scale.y)), Abs(localTF->Scale.z));
                    worldBounds->Radius = localBounds->Radius * maxScale;
                }
            }

            auto adjIt = adjacencyList.find(entt::to_integral(entity));
            if (adjIt != adjacencyList.end())
                for (entt::entity child : adjIt->second.Children)
                    processEntity(child, worldMtx);
        };

        for (entt::entity root : rootView)
            processEntity(root, float4x4{identity});
    }
}
