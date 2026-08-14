#pragma once

#include "Editor/Commands/CommandHistory.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include <EASTL/string.h>

namespace Editor::Commands
{
    class FSetNameCommand final : public IEditCommand
    {
    public:
        FSetNameCommand(Scene::ECS::FEntitySubsystem& subsystem, entt::entity entity,  const char* before, const char* after)
            : m_Subsystem(subsystem), m_Entity(entity), m_Before(before), m_After(after) {}

        void Undo() override { Apply(m_Before); }
        void Redo() override { Apply(m_After); }

    private:
        void Apply(const eastl::string& name)
        {
            auto& reg = m_Subsystem.GetRegistry();
            if (!reg.valid(m_Entity))
                return;
            if (auto* nameComp = reg.try_get<Scene::ECS::FNameComponent>(m_Entity))
                nameComp->SetName(name.c_str());
        }

    private:
        Scene::ECS::FEntitySubsystem& m_Subsystem;
        entt::entity m_Entity;
        eastl::string m_Before;
        eastl::string m_After;
    };
}