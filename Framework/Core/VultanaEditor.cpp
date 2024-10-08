#include "VultanaEditor.hpp"
#include "Core/VultanaEngine.hpp"
#include "Core/VultanaGUI.hpp"
#include "Renderer/RendererBase.hpp"

#include <imgui.h>
#include <ImGuizmo.h>

namespace Core
{
    VultanaEditor::VultanaEditor()
    {
        // std::string assetPath = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath();
        // Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
    }

    VultanaEditor::~VultanaEditor()
    {
    }

    void VultanaEditor::Tick()
    {
        FlushPendingTextureDeletions();
        mCommands.clear();

        DrawMenu();
        DrawFrameStats();
    }

    void VultanaEditor::AddGUICommand(const std::string &window, const std::string &section, const std::function<void()> &command)
    {
        mCommands[window].push_back({ section, command });
    }

    void VultanaEditor::DrawMenu()
    {
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Debug"))
            {
                if (ImGui::MenuItem("Reload Shaders"))
                {
                    pRenderer->ReloadShaders();
                }

                ImGui::EndMenu();   // End Debug Menu
            }
            
            // ImGui::MenuItem("ImGui Demo", "", &mbShowImGuiDemo);

            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("Inspector", "", &mbShowInspector);
                ImGui::MenuItem("Settings", "", &mbShowSettings);
                
                ImGui::EndMenu();   // End Window Menu
            }
            ImGui::EndMainMenuBar();
        }

        if (mbShowImGuiDemo)
        {
            ImGui::ShowDemoWindow(&mbShowImGuiDemo);
        }

        if (mbShowInspector)
        {
            Core::VultanaEngine::GetEngineInstance()->GetGUI()->AddCommand([&]() { DrawWindow("Inspector", &mbShowInspector); });
        }
        if (mbShowSettings)
        {
            Core::VultanaEngine::GetEngineInstance()->GetGUI()->AddCommand([&]() { DrawWindow("Settings", &mbShowSettings); });
        }
    }

    void VultanaEditor::DrawFrameStats()
    {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 200.0f, 50.0f));
        ImGui::SetNextWindowSize(ImVec2(200.0f, 50.0f));
        ImGui::Begin("Frame Stats", nullptr, 
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | 
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    void VultanaEditor::FlushPendingTextureDeletions()
    {
        for (size_t i = 0; i < mPendingDeletions.size(); i++)
        {
            // RHI::RHIDescriptor* srv = mPendingDeletions[i];
        }
        mPendingDeletions.clear();
    }

    void VultanaEditor::DrawWindow(const std::string &window, bool *pOpen)
    {
        ImGui::Begin(window.c_str(), pOpen);

        auto iter = mCommands.find(window);
        if (iter != mCommands.end())
        {
            for (size_t i = 0; i < iter->second.size(); i++)
            {
                const Command& cmd = iter->second[i];
                if (ImGui::CollapsingHeader(cmd.Section.c_str()))
                {
                    cmd.Function();
                }
            }
        }
        ImGui::End();
    }
}