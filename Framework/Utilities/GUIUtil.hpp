#pragma once

#include "Core/VultanaEngine.hpp"
#include "Editor/VultanaEditor.hpp"
#include <imgui.h>

inline void GUICommand(const eastl::string& window, const eastl::string& section, const eastl::function<void()>& cmd)
{
    Editor::VultanaEditor* pEditor = Core::VultanaEngine::GetEngineInstance()->GetEditor();
    pEditor->AddGUICommand(window, section, cmd);
}