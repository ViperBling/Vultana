#pragma once

#include "Editor/Commands/TransformCommand.hpp"
#include "Scene/SceneComponent/IVisibleObject.hpp"

#include <cstdint>

namespace Editor::Commands
{
    class FObjectEditableTarget final : public IEditableTarget
    {
    public:
        explicit FObjectEditableTarget(Scene::IVisibleObject& object) : m_pObject(&object) {}

        FTransformSnapshot GetTransform() const override
        {
            FTransformSnapshot snapshot;
            snapshot.Position = m_pObject->GetPosition();
            snapshot.Rotation = m_pObject->GetRotation();
            snapshot.Scale = m_pObject->GetScale();
            return snapshot;
        }

        void SetTransform(const FTransformSnapshot& snapshot) override
        {
            m_pObject->SetPosition(snapshot.Position);
            m_pObject->SetRotation(snapshot.Rotation);
            m_pObject->SetScale(snapshot.Scale);
        }

        uintptr_t GetTargetId() const override
        {
            return reinterpret_cast<uintptr_t>(m_pObject);
        }

    private:
        Scene::IVisibleObject* m_pObject;
    };
}