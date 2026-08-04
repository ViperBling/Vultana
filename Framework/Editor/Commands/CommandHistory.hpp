#pragma once

#include "Utilities/Math.hpp"

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/memory.h>

namespace Editor::Commands
{
    class IEditCommand
    {
    public:
        virtual ~IEditCommand() = default;
        virtual void Undo() = 0;
        virtual void Redo() = 0;
        virtual bool CanMerge(const IEditCommand& other) const { return false; }
        virtual void MergeWith(const IEditCommand& other) {}
    };

    class FCommandHistory
    {
    public:
        explicit FCommandHistory(uint32_t maxDepth = 100) : m_MaxDepth(maxDepth) {}

        void Push(eastl::unique_ptr<IEditCommand>&& command);
        bool Undo();
        bool Redo();
        void Clear();

        uint32_t GetUndoCount() const { return static_cast<uint32_t>(m_UndoStack.size()); }
        uint32_t GetRedoCount() const { return static_cast<uint32_t>(m_RedoStack.size()); }

    private:
        eastl::vector<eastl::unique_ptr<IEditCommand>> m_UndoStack;
        eastl::vector<eastl::unique_ptr<IEditCommand>> m_RedoStack;
        uint32_t m_MaxDepth;
    };
}