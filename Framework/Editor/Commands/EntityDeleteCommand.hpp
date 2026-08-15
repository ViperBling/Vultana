#pragma once

#include "Editor/Commands/CommandHistory.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/EntityFactory.hpp"
#include "Scene/ECS/AssetRegistry.hpp"
#include "AssetManager/ModelLoaderECS.hpp"
#include "Utilities/Log.hpp"

#include <entt/entt.hpp>

namespace Editor::Commands
{
    class FDeleteEntityCommand final : public IEditCommand
    {
    public:
        struct FModelDesc
        {
            Scene::ECS::FModelAssetHandle Asset;
            eastl::string ModelPath;
            float3 Position = float3(0.0f);
            float4 Rotation = float4(0.0f, 0.0f, 0.0f, 1.0f);
            float3 Scale = float3(1.0f);
            eastl::string Name;
            entt::entity Parent = entt::null;
        };

        FDeleteEntityCommand(Scene::ECS::FEntitySubsystem& subsystem, Scene::ECS::FEntityFactory& factory, Scene::ECS::IAssetRegistry& registry, entt::entity entity);
        void Undo() override { Restore(); }
        void Redo() override { Destroy(); }

        static FModelDesc CaptureModelDesc(Scene::ECS::FEntitySubsystem& subsystem, entt::entity root);
        
    
    private:
        void Destroy();
        void Restore();
        void RebuildModelInstance();
        void ReacquireMesh();

    private : Scene::ECS::FEntitySubsystem& m_Subsystem;
        Scene::ECS::FEntityFactory& m_Factory;
        Scene::ECS::IAssetRegistry& m_Registry;
        entt::entity m_Entity;
        bool m_bModelInstance = false;
        FModelDesc m_ModelDesc;
        Scene::ECS::FEntitySubsystem::FEntitySnapshot m_Snapshot;
    };
}