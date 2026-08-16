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

        // Shared with the persisted scene-override path (EntityFactory::ApplyMeshOverride):
        // acquire first, validate skinned guards, release old handles, install the
        // full component set and mark dirty.
        Scene::ECS::FEntityFactory::ApplyMeshKey(m_Subsystem, m_Registry, m_Entity, key.Asset, key.NodeIndex, key.PrimitiveIndex);
    }
}
