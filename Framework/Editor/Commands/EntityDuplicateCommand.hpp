#pragma once

#include "Editor/Commands/CommandHistory.hpp"
#include "Editor/Commands/EntityDeleteCommand.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/EntityFactory.hpp"
#include "Scene/ECS/AssetRegistry.hpp"
#include "AssetManager/ModelLoaderECS.hpp"
#include "Utilities/Log.hpp"

#include <entt/entt.hpp>

namespace Editor::Commands
{
    class FDuplicateCommand final : public IEditCommand
    {
    public:
        FDuplicateCommand(Scene::ECS::FEntitySubsystem& subsystem, Scene::ECS::FEntityFactory& factory, Scene::ECS::IAssetRegistry& registry, entt::entity entity);

        void Undo() override { DestroyDuplicate(); }
        void Redo() override { CreateDuplicate(); }

        entt::entity GetDuplicate() const { return m_Duplicate; }

    private:
        void CreateDuplicate();
        void DestroyDuplicate();
        void ReacquireMesh();

    private:
        Scene::ECS::FEntitySubsystem& m_Subsystem;
        Scene::ECS::FEntityFactory& m_Factory;
        Scene::ECS::IAssetRegistry& m_Registry;
        entt::entity m_Entity;
        bool m_bModelInstance = false;
        FDeleteEntityCommand::FModelDesc m_ModelDesc;
        Scene::ECS::FEntitySubsystem::FEntitySnapshot m_Snapshot;
        entt::entity m_Duplicate = entt::null;
    };
} // namespace Editor::Commands