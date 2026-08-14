#include "Scene/ECS/EntitySubsystem.hpp"
#include "Utilities/Log.hpp"

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
        const uint32_t pickID = AcquirePickID();
        m_Registry.emplace<FPickIDComponent>(entity, pickID);
        if (pickID != 0)
            m_PickIDToEntity[pickID] = entity;
        MarkStructureDirty();
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
        const uint32_t pickID = AcquirePickID();
        m_Registry.emplace<FPickIDComponent>(entity, pickID);
        if (pickID != 0)
            m_PickIDToEntity[pickID] = entity;
        MarkStructureDirty();
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
        FlushDeferred();
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
        auto* parentComp = m_Registry.try_get<FParentComponent>(entity);
        if (parentComp)
            snapshot.Parent = FEntitySnapshot::FParentData{ parentComp->Parent };
        auto* meshComp = m_Registry.try_get<FMeshRendererComponent>(entity);
        if (meshComp)
        {
            FEntitySnapshot::FMeshData meshData;
            meshData.MeshHandle = meshComp->MeshHandle;
            meshData.MaterialHandle = meshComp->MaterialHandle;
            meshData.bSkinned = meshComp->bSkinned;
            snapshot.Mesh = meshData;
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
            m_Registry.emplace_or_replace<FWorldTransformComponent>(entity);
            MarkTransformDirty(entity);
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
        if (snapshot.Mesh.has_value())
        {
            const auto& meshData = snapshot.Mesh.value();
            m_Registry.emplace_or_replace<FMeshRendererComponent>(entity, meshData.MeshHandle, meshData.MaterialHandle, meshData.bSkinned);
        }
        else if (m_Registry.all_of<FMeshRendererComponent>(entity))
        {
            m_Registry.remove<FMeshRendererComponent>(entity);
        }
        // Restore hierarchy LAST through SetParent: every call re-validates the
        // ancestor chain, so restoring A->B and B->A in either order cannot leave
        // a cycle behind (one of the links is always rejected).
        if (snapshot.Parent.has_value())
        {
            SetParent(entity, snapshot.Parent.value().Parent);
        }
        else if (m_Registry.all_of<FParentComponent>(entity))
        {
            // Snapshot had no parent — restore root state (undo of "attach to parent");
            // DetachFromParent keeps m_ChildrenMap coherent.
            DetachFromParent(entity);
        }
    }

    void FEntitySubsystem::ResurrectEntity(entt::entity entity)
    {
        if (m_Registry.valid(entity)) return;
        [[maybe_unused]] entt::entity resurrected = m_Registry.create(entity);
        MarkStructureDirty();
    }

    void FEntitySubsystem::FlushDeferred()
    {
        FlushPendingDestroy();
    }

    void FEntitySubsystem::SetParent(entt::entity entity, entt::entity parent)
    {
        if (!m_Registry.valid(entity)) return;
        if (entity == parent) return;

        // Reject attaching to any descendant: breaks cycles at the source. The
        // HierarchyProcessor sweep stays as a last-resort backstop for raw
        // registry edits, but interactive paths can no longer create cycles.
        // The depth cap mirrors HierarchyProcessor.cpp's walk: a pre-existing
        // raw-registry cycle in the ancestor chain must not hang us.
        constexpr uint32_t MAX_ANCESTOR_DEPTH = 4096;
        entt::entity anc = parent;
        for (uint32_t depth = 0; anc != entt::null && depth < MAX_ANCESTOR_DEPTH; ++depth)
        {
            if (anc == entity)
            {
                VTNA_LOG_WARN("SetParent: rejected cycle-forming parent for entity {}", entt::to_integral(entity));
                return;
            }
            const auto* ancParent = m_Registry.try_get<FParentComponent>(anc);
            anc = (ancParent != nullptr) ? ancParent->Parent : entt::null;
        }
        if (anc != entt::null)
        {
            // Depth cap hit => corrupt ancestor chain; refuse to extend it.
            VTNA_LOG_ERROR("SetParent: ancestor chain exceeds {} levels (corrupt hierarchy), rejecting", MAX_ANCESTOR_DEPTH);
            return;
        }

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
            if (!m_Registry.valid(parent))
            {
                if (m_Registry.all_of<FParentComponent>(entity))
                    m_Registry.remove<FParentComponent>(entity);
                return;
            }
            m_Registry.emplace_or_replace<FParentComponent>(entity, parent);
            m_ChildrenMap[entt::to_integral(parent)].push_back(entity);
        }
        MarkStructureDirty();
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
        MarkStructureDirty();
    }

    void FEntitySubsystem::DetachFromParent(entt::entity entity)
    {
        NotifyParentRemoved(entity);
        m_Registry.remove<FParentComponent>(entity);
        MarkStructureDirty();
    }

    void FEntitySubsystem::MarkTransformDirty(entt::entity entity)
    {
        if (auto* tf = m_Registry.try_get<FTransformComponent>(entity))
            tf->bLocalDirty = true;
        m_bTransformDirty = true;
    }

    bool FEntitySubsystem::ConsumeStructureDirty()
    {
        const bool b = m_bStructureDirty;
        m_bStructureDirty = false;
        return b;
    }

    bool FEntitySubsystem::ConsumeTransformDirty()
    {
        const bool b = m_bTransformDirty;
        m_bTransformDirty = false;
        return b;
    }

    void FEntitySubsystem::Clear()
    {
        auto& storage = m_Registry.storage<entt::entity>();
        for (auto e : storage)
            m_PendingDestroy.push_back(e);
        FlushPendingDestroy();
        m_PickIDToEntity.clear();
        m_ChildrenMap.clear();
        m_NextPickID = 1;
        MarkStructureDirty();
    }

    eastl::vector<entt::entity> FEntitySubsystem::GetChildren(entt::entity entity) const
    {
        auto it = m_ChildrenMap.find(entt::to_integral(entity));
        if (it != m_ChildrenMap.end())
            return it->second;
        return {};
    }

    const eastl::vector<entt::entity>* FEntitySubsystem::FindChildren(entt::entity entity) const
    {
        auto it = m_ChildrenMap.find(entt::to_integral(entity));
        return it != m_ChildrenMap.end() ? &it->second : nullptr;
    }

    uint32_t FEntitySubsystem::GetEntityCount() const
    {
        const auto* entityStorage = m_Registry.storage<entt::entity>();
        return static_cast<uint32_t>(entityStorage->free_list());
    }

    void FEntitySubsystem::FlushPendingDestroy()
    {
        if (m_PendingDestroy.empty()) return;
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
        MarkStructureDirty();
        m_PendingDestroy.clear();
    }

    uint32_t FEntitySubsystem::AcquirePickID()
    {
        for (uint32_t attempt = 0; attempt < MAX_PICK_ID; ++attempt)
        {
            uint32_t pickID = m_NextPickID++;
            if (m_NextPickID > MAX_PICK_ID)
                m_NextPickID = 1;

            auto iter = m_PickIDToEntity.find(pickID);
            if (iter == m_PickIDToEntity.end())
                return pickID;

            if (!m_Registry.valid(iter->second))
            {
                m_PickIDToEntity.erase(iter);
                return pickID;
            }
        }
        VTNA_LOG_ERROR("FEntitySubsystem: PickID pool exhausted (max {})", MAX_PICK_ID);
        return 0;
    }
} // namespace Scene::ECS
