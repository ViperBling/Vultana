#include "Scene/ECS/EntitySubsystem.hpp"
#include "Utilities/Log.hpp"
#include "EntitySubsystem.hpp"

namespace Scene::ECS
{
    FEntitySubsystem::FEntitySubsystem()
    {
        VTNA_LOG_INFO("FEntitySubsystem initialized");
    }

    FEntitySubsystem::~FEntitySubsystem()
    {
        FlushPendingDestroy();
    }

    entt::entity FEntitySubsystem::CreateEntity()
    {
        entt::entity entity = m_Registry.create();
        uint32_t pickID = m_NextPickID++;
        if (pickID >= MAX_PICK_ID) 
        {
            pickID = 1;
            m_NextPickID = 2;
        }
        m_Registry.emplace<FPickIDComponent>(entity, pickID);
        m_PickIDToEntity[pickID] = entity;
        return entity;
    }

    void FEntitySubsystem::DestroyEntityDeferred(entt::entity entity)
    {
        if (!m_Registry.valid(entity))
            return;
        auto* pickIDComp = m_Registry.try_get<FPickIDComponent>(entity);
        if (pickIDComp)
            m_PickIDToEntity.erase(pickIDComp->PickID);
        m_PendingDestroy.push_back(entity);
    }

    entt::entity FEntitySubsystem::CreateEntityAt(entt::entity specEntity)
    {
        entt::entity entity = m_Registry.create(specEntity);
        uint32_t pickID = m_NextPickID++;
        if (pickID >= MAX_PICK_ID) 
        {
            pickID = 1;
            m_NextPickID = 2;
        }
        m_Registry.emplace<FPickIDComponent>(entity, pickID);
        m_PickIDToEntity[pickID] = entity;
        return entity;
    }

    entt::entity FEntitySubsystem::ResolvePickID(uint32_t pickID) const
    {
        auto iter = m_PickIDToEntity.find(pickID);
        if (iter == m_PickIDToEntity.end()) 
            return entt::null;
        entt::entity entity = iter->second;
        if (!m_Registry.valid(entity)) 
            return entt::null;
        return entity;
    }

    uint32_t FEntitySubsystem::GetPickID(entt::entity entity) const
    {
        auto* pickIDComp = m_Registry.try_get<FPickIDComponent>(entity);
        return pickIDComp ? pickIDComp->PickID : 0;
    }

    void FEntitySubsystem::RegisterProcessor(eastl::unique_ptr<FProcessorBase> processor)
    {
        if (!processor) return;
        size_t phase = static_cast<size_t>(processor->GetProcessingPhase());
        if (phase >= static_cast<size_t>(EProcessingPhase::Count))
        {
            VTNA_LOG_ERROR("Invalid processing phase for: {}", processor->GetName());
            return;
        }
        m_Processors[phase].push_back(eastl::move(processor));
    }

    void FEntitySubsystem::Tick(float deltaTime)
    {
        FlushPendingDestroy();
        for (size_t phase = 0; phase < static_cast<size_t>(EProcessingPhase::Count); ++phase)
        {
            for (auto& processor : m_Processors[phase])
            {
                processor->Process(*this, deltaTime);
            }
        }
    }

    FEntitySubsystem::FEntitySnapshot FEntitySubsystem::SnapEntity(entt::entity entity) const
    {
        FEntitySnapshot snapshot;
        snapshot.Entity = entity;
        auto* pickID = m_Registry.try_get<FPickIDComponent>(entity);
        snapshot.PickID = pickID ? pickID->PickID : 0;
        auto* transform = m_Registry.try_get<FTransformComponent>(entity);
        if (transform)
        {
            FEntitySnapshot::FTransformData transformData;
            transformData.Position = transform->Position;
            transformData.Rotation = transform->Rotation;
            transformData.Scale = transform->Scale;
            snapshot.Transform = transformData;
        }
        auto* name = m_Registry.try_get<FNameComponent>(entity);
        if (name)
        {
            FEntitySnapshot::FNameData nameData;
            memcpy(nameData.Name, name->Name, sizeof(nameData.Name));
            snapshot.Name = nameData;
        }
        return snapshot;
    }

    void FEntitySubsystem::ApplyEntitySnapshot(const FEntitySnapshot &snapshot)
    {
        entt::entity entity = snapshot.Entity;
        if (!m_Registry.valid(entity))
        {
            ResurrectEntity(entity);
            if (snapshot.PickID > 0)
            {
                m_Registry.emplace<FPickIDComponent>(entity, snapshot.PickID);
                m_PickIDToEntity[snapshot.PickID] = entity;
            }
        }
        if (snapshot.Transform.has_value())
        {
            const auto& transformData = snapshot.Transform.value();
            m_Registry.emplace_or_replace<FTransformComponent>(entity, transformData.Position, transformData.Rotation, transformData.Scale);
        }
        if (snapshot.Name.has_value())
        {
            m_Registry.emplace_or_replace<FNameComponent>(entity);
            auto* nameComp = m_Registry.try_get<FNameComponent>(entity);
            if (nameComp)
            {
                memcpy(nameComp->Name, snapshot.Name.value().Name, sizeof(nameComp->Name));
            }
        }
    }

    void FEntitySubsystem::ResurrectEntity(entt::entity entity)
    {
        if (m_Registry.valid(entity)) return;
        [[maybe_unused]] entt::entity resurrected = m_Registry.create(entity);
    }

    void FEntitySubsystem::SetParent(entt::entity entity, entt::entity parent)
    {
        if (!m_Registry.valid(entity)) return;
        if (entity == parent) return;
        if (parent == entt::null)
        {
            m_Registry.remove<FParentComponent>(entity);
        }
        else
        {
            if (!m_Registry.valid(parent)) return;
            m_Registry.emplace_or_replace<FParentComponent>(entity, parent);
        }
    }

    eastl::vector<entt::entity> FEntitySubsystem::GetChildren(entt::entity entity) const
    {
        eastl::vector<entt::entity> children;
        auto view = m_Registry.view<FParentComponent>();
        for (auto e : view)
        {
            if (view.get<FParentComponent>(e).Parent == entity)
                children.push_back(e);
        }
        return children;
    }

    uint32_t FEntitySubsystem::GetEntityCount() const
    {
        const auto* entityStorage = m_Registry.storage<entt::entity>();
        return static_cast<uint32_t>(entityStorage->free_list());
    }

    void FEntitySubsystem::FlushPendingDestroy()
    {
        for (entt::entity entity : m_PendingDestroy)
        {
            if (m_Registry.valid(entity)) m_Registry.destroy(entity);
        }
        m_PendingDestroy.clear();
    }
}
