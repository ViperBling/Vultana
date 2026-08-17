#include "Editor/Commands/EntityDeleteCommand.hpp"

namespace Editor::Commands
{
    Editor::Commands::FDeleteEntityCommand::FDeleteEntityCommand(Scene::ECS::FEntitySubsystem& subsystem, Scene::ECS::FEntityFactory& factory, Scene::ECS::IAssetRegistry& registry, entt::entity entity)
        : m_Subsystem(subsystem), m_Factory(factory), m_Registry(registry), m_Entity(entity)
    {
        m_bModelInstance = subsystem.GetRegistry().all_of<Scene::ECS::FModelInstanceComponent>(entity);
        if (m_bModelInstance)
        {
            m_ModelDesc = CaptureModelDesc(subsystem, entity);
        }
        else
        {
            m_Snapshot = subsystem.SnapEntity(entity);
        }
    }

    FDeleteEntityCommand::FModelDesc Editor::Commands::FDeleteEntityCommand::CaptureModelDesc(Scene::ECS::FEntitySubsystem& subsystem, entt::entity root)
    {
        FModelDesc desc;
        auto& reg = subsystem.GetRegistry();
        if (const auto* model = reg.try_get<Scene::ECS::FModelInstanceComponent>(root))
        {
            desc.Asset = model->ModelHandle;
            desc.ModelPath = model->ModelPath;
        }
        if (const auto* tf = reg.try_get<Scene::ECS::FTransformComponent>(root))
        {
            desc.Position = tf->Position;
            desc.Rotation = tf->Rotation;
            desc.Scale = tf->Scale;
        }
        if (const auto* nameComp = reg.try_get<Scene::ECS::FNameComponent>(root))
            desc.Name = nameComp->Name;
        if (const auto* parentComp = reg.try_get<Scene::ECS::FParentComponent>(root))
            desc.Parent = parentComp->Parent;
        return desc;
    }

    void FDeleteEntityCommand::Destroy()
    {
        auto& reg = m_Subsystem.GetRegistry();
        if (!reg.valid(m_Entity))
            return;
        m_Factory.DestroyModelInstance(m_Entity);
        m_Subsystem.FlushDeferred();
    }

    void FDeleteEntityCommand::Restore()
    {
        if (m_bModelInstance)
        {
            RebuildModelInstance();
        }
        else
        {
            m_Subsystem.ApplyEntitySnapshot(m_Snapshot);
            ReacquireMesh();
        }
    }
    
    void FDeleteEntityCommand::RebuildModelInstance()
    {
        auto& reg = m_Subsystem.GetRegistry();
        if (m_Entity != entt::null && reg.valid(m_Entity))
            return; 
        if (!m_ModelDesc.Asset.IsValid())
            return;

        if (m_Registry.GetState(m_ModelDesc.Asset) != Scene::ECS::EAssetState::Ready)
        {
            VTNA_LOG_WARN("[FDeleteEntityCommand] undo: asset {} not ready; cannot rebuild", m_ModelDesc.Asset.Index);
            return;
        }
        const Scene::ECS::FModelAssetInfo info = m_Registry.GetModelInfo(m_ModelDesc.Asset);
        entt::entity root = m_Factory.CreateModelInstance(m_ModelDesc.Asset, m_ModelDesc.ModelPath);
        m_Factory.CreateModelHierarchy(m_ModelDesc.Asset, info, root, m_ModelDesc.Position, m_ModelDesc.Rotation, m_ModelDesc.Scale);
        
        if (auto* nameComp = reg.try_get<Scene::ECS::FNameComponent>(root))
            nameComp->SetName(m_ModelDesc.Name.c_str());
        if (m_ModelDesc.Parent != entt::null && reg.valid(m_ModelDesc.Parent))
            m_Subsystem.SetParent(root, m_ModelDesc.Parent);

        m_Entity = root;
    }

    void FDeleteEntityCommand::ReacquireMesh()
    {
        if (!m_Snapshot.Mesh.has_value())
            return;

        auto& reg = m_Subsystem.GetRegistry();
        if (!reg.valid(m_Entity))
            return;

        const auto& mesh = m_Snapshot.Mesh.value();
        if (!mesh.Asset.IsValid() || mesh.NodeIndex < 0 || mesh.PrimitiveIndex < 0)
        {
            reg.remove<Scene::ECS::FMeshRendererComponent>(m_Entity);
            return;
        }

        const Scene::ECS::FMeshResourceHandle mh = m_Registry.AcquireMesh(mesh.Asset, static_cast<uint32_t>(mesh.NodeIndex), static_cast<uint32_t>(mesh.PrimitiveIndex));
        const Scene::ECS::FMaterialResourceHandle mat = m_Registry.AcquireMaterial(mesh.Asset, static_cast<uint32_t>(mesh.NodeIndex), static_cast<uint32_t>(mesh.PrimitiveIndex));
        
        if (!mh.IsValid() || !mat.IsValid())
        {
            VTNA_LOG_WARN("[FDeleteEntityCommand] re-acquire failed on undo (asset {} node {} prim {})", mesh.Asset.Index, mesh.NodeIndex, mesh.PrimitiveIndex);
            reg.remove<Scene::ECS::FMeshRendererComponent>(m_Entity);
            return;
        }

        const Scene::ECS::FMeshGPUData md = m_Registry.GetMeshData(mh);
        const entt::entity root = md.bSkinned ? Assets::FModelLoaderECS::FindModelInstanceRoot(reg, m_Entity) : entt::null;
        Scene::ECS::FEntityFactory::SetMeshRenderer(reg, m_Entity, mh, mat, md, mesh.Asset, mesh.NodeIndex, mesh.PrimitiveIndex, root);
    }
} // namespace Editor::Commands
