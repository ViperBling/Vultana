#pragma once

#include "Core/VultanaEngine.hpp"
#include "Core/VultanaEditor.hpp"
#include <imgui.h>

inline void GUICommand(const std::string& window, const std::string& section, const std::function<void()>& cmd)
{
    Core::VultanaEditor* pEditor = Core::VultanaEngine::GetEngineInstance()->GetEditor();
    pEditor->AddGUICommand(window, section, cmd);
}