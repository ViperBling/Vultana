#include "VultanaGUI.hpp"

#include "VultanaEngine.hpp"

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>

namespace Core
{
    GUI::GUI()
    {
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
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
        // for (size_t i = 0; i < mCommands.size(); i++)
        // {
        //     mCommands[i]();
        // }
        // mCommands.clear();

        // ImGui::Render();
    }

    void GUI::SetupRenderStates(RHI::RHICommandList *pCmdList, uint32_t frameIdx)
    {
        // ImDrawData* drawData = ImGui::GetDrawData();

        // pCmdList->SetViewport(0, 0, 
        //     (uin32_t)(drawData->DisplaySize.x * drawData->FramebufferScale.x),
        //     (uin32_t)(drawData->DisplaySize.y * drawData->FramebufferScale.y));
        // pCmdList->SetPipelineState(mpPSO);
        // pCmdList->SetIndexBuffer(mpIndexBuffer[frameIdx]->GetBuffer(), 0, mpIndexBuffer[frameIdx]->GetFormat());

        // TODO
    }
}