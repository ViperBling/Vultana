#pragma once

#include "Core/VultanaEngine.hpp"
#include "Core/VultanaEditor.hpp"
#include <imgui.h>

inline void GUICommand(const eastl::string& window, const eastl::string& section, const eastl::function<void()>& cmd)
{
    Core::VultanaEditor* pEditor = Core::VultanaEngine::GetEngineInstance()->GetEditor();
    pEditor->AddGUICommand(window, section, cmd);
}