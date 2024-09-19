#include "VultanaGUI.hpp"

#include "VultanaEngine.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>

namespace Core
{
    GUI::GUI()
    {
        ImGui::CreateContext();

        ImGui_ImplWin32_Init(Core::VultanaEngine::GetEngineInstance()->GetWindowHandle()->GetNativeHandle());
    }

    GUI::~GUI()
    {
        ImGui_ImplWin32_Shutdown();
    }

    bool GUI::Init()
    {
        return true;
    }

    void GUI::Tick()
    {
    }
}