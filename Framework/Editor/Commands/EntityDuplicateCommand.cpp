#include "Editor/Commands/EntityDuplicateCommand.hpp"

namespace Editor::Commands
{
    FDuplicateCommand::FDuplicateCommand(Scene::ECS::FEntitySubsystem& subsystem, Scene::ECS::FEntityFactory& factory, Scene::ECS::IAssetRegistry& registry, entt::entity entity)
        : m_Subsystem(subsystem), m_Factory(factory), m_Registry(registry), m_Entity(entity)
    {
        m_bModelInstance = subsystem.GetRegistry().all_of<Scene::ECS::FModelInstanceComponent>(entity);
        if (m_bModelInstance)
            m_ModelDesc = FDeleteEntityCommand::CaptureModelDesc(subsystem, entity);
        else
            m_Snapshot = subsystem.SnapEntity(entity);
    }

    void FDuplicateCommand::CreateDuplicate()
    {
        auto& reg = m_Subsystem.GetRegistry();
        if (m_Duplicate != entt::null && reg.valid(m_Duplicate))
            return;

        if (m_bModelInstance)
        {
            if (!m_ModelDesc.Asset.IsValid())
                return;
            if (m_Registry.GetState(m_ModelDesc.Asset) != Scene::ECS::EAssetState::Ready)
            {
                VTNA_LOG_WARN("[FDuplicateCommand] asset {} not ready; cannot duplicate", m_ModelDesc.Asset.Index);
                return;
            }
            const Scene::ECS::FModelAssetInfo info = m_Registry.GetModelInfo(m_ModelDesc.Asset);
            m_Duplicate = m_Factory.CreateModelInstance(m_ModelDesc.Asset, m_ModelDesc.ModelPath);
            m_Factory.CreateModelHierarchy(m_ModelDesc.Asset, info, m_Duplicate, m_ModelDesc.Position, m_ModelDesc.Rotation, m_ModelDesc.Scale);
            
            if (auto* nameComp = reg.try_get<Scene::ECS::FNameComponent>(m_Duplicate))
                nameComp->SetName(m_ModelDesc.Name.c_str());
            if (m_ModelDesc.Parent != entt::null && reg.valid(m_ModelDesc.Parent))
                m_Subsystem.SetParent(m_Duplicate, m_ModelDesc.Parent);
        }
        else
        {
            m_Duplicate = m_Subsystem.CreateEntity();
            Scene::ECS::FEntitySubsystem::FEntitySnapshot copy = m_Snapshot;
            copy.Entity = m_Duplicate;
            copy.PickID = 0;
            m_Subsystem.ApplyEntitySnapshot(copy);
            ReacquireMesh();
        }
        // Nudge so the duplicate is visibly offset from the source.
        if (auto* tf = reg.try_get<Scene::ECS::FTransformComponent>(m_Duplicate))
            tf->Position.x += 0.5f;
        
        m_Subsystem.MarkTransformDirty(m_Duplicate);
    }

    void FDuplicateCommand::DestroyDuplicate()
    {
        auto& reg = m_Subsystem.GetRegistry();
        if (m_Duplicate == entt::null || !reg.valid(m_Duplicate))
            return;

        m_Factory.DestroyModelInstance(m_Duplicate);
        m_Subsystem.FlushDeferred();
    }

    void FDuplicateCommand::ReacquireMesh()
    {
        if (!m_Snapshot.Mesh.has_value())
            return;

        const auto& mesh = m_Snapshot.Mesh.value();
        auto& reg = m_Subsystem.GetRegistry();

        if (!mesh.Asset.IsValid() || mesh.NodeIndex < 0 || mesh.PrimitiveIndex < 0)
        {
            reg.remove<Scene::ECS::FMeshRendererComponent>(m_Duplicate);
            return;
        }
        const Scene::ECS::FMeshResourceHandle mh = m_Registry.AcquireMesh(mesh.Asset, static_cast<uint32_t>(mesh.NodeIndex), static_cast<uint32_t>(mesh.PrimitiveIndex));
        const Scene::ECS::FMaterialResourceHandle mat = m_Registry.AcquireMaterial(mesh.Asset, static_cast<uint32_t>(mesh.NodeIndex), static_cast<uint32_t>(mesh.PrimitiveIndex));
        
        if (!mh.IsValid() || !mat.IsValid())
        {
            VTNA_LOG_WARN("[FDuplicateCommand] re-acquire failed (asset {} node {} prim {})", mesh.Asset.Index, mesh.NodeIndex, mesh.PrimitiveIndex);

            reg.remove<Scene::ECS::FMeshRendererComponent>(m_Duplicate);
            return;
        }

        const Scene::ECS::FMeshGPUData md = m_Registry.GetMeshData(mh);
        reg.emplace_or_replace<Scene::ECS::FMeshRendererComponent>(m_Duplicate, mh, mat, md.bSkinned);
        reg.emplace_or_replace<Scene::ECS::FBoundingSphereComponent>(m_Duplicate, md.Center, md.Radius);
        reg.emplace_or_replace<Scene::ECS::FWorldBoundingSphereComponent>(m_Duplicate);
        reg.emplace_or_replace<Scene::ECS::FVisibilityComponent>(m_Duplicate, true);

        if (md.bSkinned)
        {
            const entt::entity root = Assets::FModelLoaderECS::FindModelInstanceRoot(reg, m_Duplicate);
            if (root != entt::null)
                reg.emplace_or_replace<Scene::ECS::FSkinnedRootComponent>(m_Duplicate, root);
        }

        if (auto* mc = reg.try_get<Scene::ECS::FMeshRendererComponent>(m_Duplicate))
        {
            mc->Asset = mesh.Asset;
            mc->NodeIndex = mesh.NodeIndex;
            mc->PrimitiveIndex = mesh.PrimitiveIndex;
        }
    }
}

