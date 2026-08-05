#pragma once

#include "Editor/Commands/TransformCommand.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Fragments.hpp"
#include <cstdint>

namespace Editor::Commands
{
    class FEntityEditableTarget final : public IEditableTarget
    {
    public:
        FEntityEditableTarget(Scene::ECS::FEntitySubsystem& entitySubsystem, entt::entity entity) : m_EntitySubsystem(entitySubsystem), m_Entity(entity) {}
        FTransformSnapshot GetTransform() const override
        {
            FTransformSnapshot snapshot;
            auto* transform = m_EntitySubsystem.GetRegistry().try_get<Scene::ECS::FTransformFragment>(m_Entity);
            if (transform)
            {
                snapshot.Position = transform->Position;
                snapshot.Rotation = quaternion(transform->Rotation.x, transform->Rotation.y, transform->Rotation.z, transform->Rotation.w);
                snapshot.Scale = transform->Scale;
            }
            return snapshot;
        }

        void SetTransform(const FTransformSnapshot &snapshot) override
        {
            m_EntitySubsystem.GetRegistry().emplace_or_replace<Scene::ECS::FTransformFragment>(
                m_Entity, snapshot.Position,
                float4(snapshot.Rotation.x, snapshot.Rotation.y, snapshot.Rotation.z, snapshot.Rotation.w),
                snapshot.Scale);
        }

        uintptr_t GetTargetId() const override { return static_cast<uintptr_t>(entt::to_integral(m_Entity)); }

    private:
        Scene::ECS::FEntitySubsystem& m_EntitySubsystem;
        entt::entity m_Entity;
    };
}