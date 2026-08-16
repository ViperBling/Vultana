#include "Editor/Commands/MeshCommand.hpp"
#include "Scene/ECS/EntityFactory.hpp"

namespace Editor::Commands
{
    FSetMeshCommand::FSetMeshCommand(Scene::ECS::FEntitySubsystem& subsystem, Scene::ECS::IAssetRegistry& registry, entt::entity entity, const FKey& before, const FKey& after)
        : m_Subsystem(subsystem), m_Registry(registry), m_Entity(entity), m_Before(before), m_After(after) {}

    void FSetMeshCommand::Undo()
    {
        Apply(m_Before);
    }

    void FSetMeshCommand::Redo()
    {
        Apply(m_After);
    }

    void FSetMeshCommand::Apply(const FKey& key)
    {
        auto& reg = m_Subsystem.GetRegistry();
        auto* mc = reg.try_get<Scene::ECS::FMeshRendererComponent>(m_Entity);
        if (mc == nullptr || !key.IsValid())
            return;

        const Scene::ECS::FMeshResourceHandle mh = m_Registry.AcquireMesh(key.Asset, static_cast<uint32_t>(key.NodeIndex), static_cast<uint32_t>(key.PrimitiveIndex));
        const Scene::ECS::FMaterialResourceHandle mat = m_Registry.AcquireMaterial(key.Asset, static_cast<uint32_t>(key.NodeIndex), static_cast<uint32_t>(key.PrimitiveIndex));
        if (!mh.IsValid() || !mat.IsValid())
        {
            if (mh.IsValid()) m_Registry.Release(mh);
            if (mat.IsValid()) m_Registry.Release(mat);
            return;
        }

        const Scene::ECS::FMeshGPUData md = m_Registry.GetMeshData(mh);
        if (md.bSkinned != mc->bSkinned || (md.bSkinned && !(key.Asset == mc->Asset)))
        {
            m_Registry.Release(mh);
            m_Registry.Release(mat);
            VTNA_LOG_WARN("[FSetMeshCommand] skinned/static or cross-asset skinned swap rejected (entity {})", entt::to_integral(m_Entity));
            return;
        }
        m_Registry.Release(mc->MeshHandle);
        m_Registry.Release(mc->MaterialHandle);
        Scene::ECS::FEntityFactory::SetMeshRenderer(reg, m_Entity, mh, mat, md, key.Asset, key.NodeIndex, key.PrimitiveIndex);
        m_Subsystem.MarkTransformDirty(m_Entity);
    }
}
