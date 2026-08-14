#pragma once

#include "Editor/Commands/CommandHistory.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include <entt/entt.hpp>

namespace Editor::Commands
{
    class FSetParentCommand final : public IEditCommand
    {
    public:
        FSetParentCommand(Scene::ECS::FEntitySubsystem& subsystem, entt::entity entity, entt::entity before, entt::entity after)
            : m_Subsystem(subsystem), m_Entity(entity), m_Before(before), m_After(after) {}

        void Undo() override { m_Subsystem.SetParent(m_Entity, m_Before); }
        void Redo() override { m_Subsystem.SetParent(m_Entity, m_After); }

    private:
        Scene::ECS::FEntitySubsystem& m_Subsystem;
        entt::entity m_Entity;
        entt::entity m_Before;
        entt::entity m_After;
    };
}