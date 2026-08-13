#pragma once

#include "Scene/ECS/ProcessorBase.hpp"
#include "Scene/ECS/Components.hpp"

#include <entt/entt.hpp>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/hash_map.h>
#include <EASTL/optional.h>
#include <cstdint>

namespace Scene::ECS
{
    class FEntitySubsystem
    {
    public:
        FEntitySubsystem();
        ~FEntitySubsystem();

        // ===== Entity Management ===== //
        entt::entity CreateEntity();
        void DestroyEntityDeferred(entt::entity entity);
        entt::entity CreateEntityAt(entt::entity specEntity);

        // ===== ID Management ===== //
        entt::entity ResolvePickID(uint32_t pickID) const;
        uint32_t GetPickID(entt::entity entity) const;

        // ===== Frame Update ===== //
        void RegisterProcessor(eastl::unique_ptr<FProcessorBase> processor);
        void Tick(float deltaTime);

        // ===== Registry Access ===== //
        entt::registry& GetRegistry() { return m_Registry; }
        const entt::registry& GetRegistry() const { return m_Registry; }

        // ===== Entity Snapshot ===== //
        struct FEntitySnapshot
        {
            entt::entity Entity;
            uint32_t PickID;

            struct FTransformData
            {
                float3 Position;
                float4 Rotation;
                float3 Scale;
            };
            eastl::optional<FTransformData> Transform;

            struct FNameData
            {
                char Name[64];
            };
            eastl::optional<FNameData> Name;
        };
        using FChildrenMap = eastl::hash_map<entt::id_type, eastl::vector<entt::entity>>;

        FEntitySnapshot SnapEntity(entt::entity entity) const;
        void ApplyEntitySnapshot(const FEntitySnapshot& snapshot);

        void ResurrectEntity(entt::entity entity);

        // ==== Layer Operations ==== //
        void SetParent(entt::entity entity, entt::entity parent);
        eastl::vector<entt::entity> GetChildren(entt::entity entity) const;
        const eastl::vector<entt::entity>* FindChildren(entt::entity entity) const;
        void NotifyParentRemoved(entt::entity entity);

        const FChildrenMap& GetChildrenMap() const { return m_ChildrenMap; }
        void DetachFromParent(entt::entity entity);

        // ===== Query Operations ===== //
        uint32_t GetEntityCount() const;

    private:
        void FlushPendingDestroy();
        uint32_t AcquirePickID();

    private:
        entt::registry m_Registry;
        eastl::vector<eastl::unique_ptr<FProcessorBase>> m_Processors[static_cast<size_t>(EProcessingPhase::Count)];
        eastl::hash_map<uint32_t, entt::entity> m_PickIDToEntity;
        uint32_t m_NextPickID = 1;
        static constexpr uint32_t MAX_PICK_ID = 9999999;
        eastl::vector<entt::entity> m_PendingDestroy;
        FChildrenMap m_ChildrenMap;
    };
}