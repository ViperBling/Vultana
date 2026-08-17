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
        if (!Scene::ECS::FEntityFactory::ApplyMeshKey(m_Subsystem, m_Registry, m_Entity, key.Asset, key.NodeIndex, key.PrimitiveIndex))
        {
            VTNA_LOG_WARN("[FSetMeshCommand] apply rejected (entity {}, node {}, prim {})", entt::to_integral(m_Entity), key.NodeIndex, key.PrimitiveIndex);
        }
    }
}
