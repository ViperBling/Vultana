#pragma once

#include "Editor/Commands/CommandHistory.hpp"
#include "Utilities/Math.hpp"

#include <EASTL/unique_ptr.h>
#include <cstdint>

namespace Editor::Commands
{
    struct FTransformSnapshot
    {
        float3 Position = float3(0.0f);
        quaternion Rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        float3 Scale = float3(1.0f);
    };

    inline bool TransformEqual(const FTransformSnapshot &a, const FTransformSnapshot &b)
    {
        return (a.Position == b.Position) && (a.Rotation == b.Rotation) && (a.Scale == b.Scale);
    }

    class IEditableTarget
    {
    public:
        virtual ~IEditableTarget() = default;
        virtual FTransformSnapshot GetTransform() const = 0;
        virtual void SetTransform(const FTransformSnapshot &snapshot) = 0;
        virtual uintptr_t GetTargetId() const { return 0; } // CanMerge 判定"同一目标"
    };

    class FSetTransformCommand final : public IEditCommand
    {
    public:
        FSetTransformCommand(eastl::unique_ptr<IEditableTarget> &&target,
                             FTransformSnapshot before, FTransformSnapshot after)
            : m_pTarget(eastl::move(target)), m_Before(before), m_After(after) {}

        void Undo() override { m_pTarget->SetTransform(m_Before); }
        void Redo() override { m_pTarget->SetTransform(m_After); }

        bool CanMerge(const IEditCommand &other) const override
        {
            const FSetTransformCommand *pOther = dynamic_cast<const FSetTransformCommand *>(&other);
            return pOther != nullptr && pOther->m_pTarget->GetTargetId() == m_pTarget->GetTargetId() && TransformEqual(pOther->m_Before, m_Before);
        }

        void MergeWith(const IEditCommand &other) override
        {
            const FSetTransformCommand *pOther = dynamic_cast<const FSetTransformCommand *>(&other);
            if (pOther != nullptr)
            {
                m_After = pOther->m_After;
            }
        }

    private:
        eastl::unique_ptr<IEditableTarget> m_pTarget;
        FTransformSnapshot m_Before;
        FTransformSnapshot m_After;
    };
}