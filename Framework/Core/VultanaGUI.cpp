#include "VultanaGUI.hpp"

#include "VultanaEngine.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>

namespace Core
{
    GUI::GUI()
    {
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(Core::VultanaEngine::GetEngineInstance()->GetWindowHandle()->GetNativeHandle());
    }

    GUI::~GUI()
    {
        ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();
    }

    bool GUI::Init()
    {
        return true;
    }

    void GUI::Tick()
    {
        // ImGui_ImplWin32_NewFrame();
        // ImGui::NewFrame();
    }

    void GUI::Render(RHI::RHICommandList *pCmdList)
    {
        // ImGui::Render();
    }
}