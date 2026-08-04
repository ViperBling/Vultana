#include "Editor/Commands/CommandHistory.hpp"

#include <EASTL/utility.h>

namespace Editor::Commands
{
    void FCommandHistory::Push(eastl::unique_ptr<IEditCommand>&& command)
    {
        if (command == nullptr) return;

        m_RedoStack.clear();

        if (!m_UndoStack.empty() && m_UndoStack.back()->CanMerge(*command))
        {
            m_UndoStack.back()->MergeWith(*command);
            return;
        }
        m_UndoStack.push_back(eastl::move(command));
        if (m_UndoStack.size() > m_MaxDepth)
        {
            m_UndoStack.erase(m_UndoStack.begin(), m_UndoStack.begin() + (m_UndoStack.size() - m_MaxDepth));
        }
    }

    bool FCommandHistory::Undo()
    {
        if (m_UndoStack.empty())
        {
            return false;
        }

        eastl::unique_ptr<IEditCommand> command = eastl::move(m_UndoStack.back());
        m_UndoStack.pop_back();
        command->Undo();
        m_RedoStack.push_back(eastl::move(command));

        return true;
    }

    bool FCommandHistory::Redo()
    {
        if (m_RedoStack.empty())
        {
            return false;
        }

        eastl::unique_ptr<IEditCommand> command = eastl::move(m_RedoStack.back());
        m_RedoStack.pop_back();
        command->Redo();
        m_UndoStack.push_back(eastl::move(command));

        return true;
    }

    void FCommandHistory::Clear()
    {
        m_UndoStack.clear();
        m_RedoStack.clear();
    }
}