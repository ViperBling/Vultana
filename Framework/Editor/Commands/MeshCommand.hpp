#pragma once

#include "Editor/Commands/CommandHistory.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/AssetRegistry.hpp"
#include "Utilities/Log.hpp"

#include <entt/entt.hpp>

namespace Editor::Commands
{
    class FSetMeshCommand final : public IEditCommand
    {
    public:
        struct FKey
        {
            Scene::ECS::FModelAssetHandle Asset;
            int32_t NodeIndex = -1;
            int32_t PrimitiveIndex = -1;
            bool IsValid() const { return Asset.IsValid() && NodeIndex >= 0 && PrimitiveIndex >= 0; }
        };

        FSetMeshCommand(Scene::ECS::FEntitySubsystem& subsystem, Scene::ECS::IAssetRegistry& registry, entt::entity entity, const FKey& before, const FKey& after);

        void Undo() override;
        void Redo() override;

    private:
        void Apply(const FKey& key);

        Scene::ECS::FEntitySubsystem& m_Subsystem;
        Scene::ECS::IAssetRegistry& m_Registry;
        entt::entity m_Entity;
        FKey m_Before;
        FKey m_After;
    };
}
