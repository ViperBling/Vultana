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

        // Remove from old parent's children list
        auto* oldParentComp = m_Registry.try_get<FParentComponent>(entity);
        if (oldParentComp)
        {
            auto oldId = entt::to_integral(oldParentComp->Parent);
            auto it = m_ChildrenMap.find(oldId);
            if (it != m_ChildrenMap.end())
            {
                it->second.erase_first(entity);
                if (it->second.empty())
                    m_ChildrenMap.erase(it);
            }
        }

        if (parent == entt::null)
        {
            m_Registry.remove<FParentComponent>(entity);
        }
        else
        {
            if (!m_Registry.valid(parent)) return;
            m_Registry.emplace_or_replace<FParentComponent>(entity, parent);
            m_ChildrenMap[entt::to_integral(parent)].push_back(entity);
        }
    }

    void FEntitySubsystem::NotifyParentRemoved(entt::entity entity)
    {
        auto* parentComp = m_Registry.try_get<FParentComponent>(entity);
        if (!parentComp) return;
        auto parentId = entt::to_integral(parentComp->Parent);
        auto it = m_ChildrenMap.find(parentId);
        if (it != m_ChildrenMap.end())
        {
            it->second.erase_first(entity);
            if (it->second.empty())
                m_ChildrenMap.erase(it);
        }
    }

    eastl::vector<entt::entity> FEntitySubsystem::GetChildren(entt::entity entity) const
    {
        auto it = m_ChildrenMap.find(entt::to_integral(entity));
        if (it != m_ChildrenMap.end())
            return it->second;
        return {};
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
            if (!m_Registry.valid(entity)) continue;

            // Remove entity from its parent's children list
            auto* parentComp = m_Registry.try_get<FParentComponent>(entity);
            if (parentComp)
            {
                auto parentId = entt::to_integral(parentComp->Parent);
                auto pit = m_ChildrenMap.find(parentId);
                if (pit != m_ChildrenMap.end())
                {
                    pit->second.erase_first(entity);
                    if (pit->second.empty())
                        m_ChildrenMap.erase(pit);
                }
            }

            // Detach all children before destroying the parent
            auto cit = m_ChildrenMap.find(entt::to_integral(entity));
            if (cit != m_ChildrenMap.end())
            {
                for (auto child : cit->second)
                    m_Registry.remove<FParentComponent>(child);
                m_ChildrenMap.erase(cit);
            }

            m_Registry.destroy(entity);
        }
        m_PendingDestroy.clear();
    }
}
