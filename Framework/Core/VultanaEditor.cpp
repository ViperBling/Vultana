#include "VultanaEditor.hpp"
#include "Core/VultanaEngine.hpp"
#include "Core/VultanaGUI.hpp"
#include "Renderer/RendererBase.hpp"

#include "ImFileDialog/ImFileDialog.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

namespace Core
{
    VultanaEditor::VultanaEditor()
    {
        ifd::FileDialog::Instance().CreateTexture = [this](uint8_t* data, int w, int h, char fmt) -> void*
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
            auto pTexture = pRenderer->CreateTexture2D(w, h, 1, fmt == 1 ? RHI::ERHIFormat::RGBA8SRGB : RHI::ERHIFormat::BGRA8SRGB, 0, "ImFileDialogIcon");
            pRenderer->UploadTexture(pTexture->GetTexture(), data);

            mFileDialogIcons.insert(std::make_pair(pTexture->GetSRV(), pTexture));

            return pTexture->GetSRV();
        };
        ifd::FileDialog::Instance().DeleteTexture = [this](void* tex)
        {
            mPendingDeletions.push_back(static_cast<RHI::RHIDescriptor*>(tex));
        };
    }

    VultanaEditor::~VultanaEditor()
    {
        for (auto iter = mFileDialogIcons.begin(); iter != mFileDialogIcons.end(); iter++)
        {
            delete iter->first;
            delete iter->second;
        }
    }

    void VultanaEditor::Tick()
    {
        FlushPendingTextureDeletions();
        mCommands.clear();

        BuildDockLayout();
        DrawMenu();
        DrawFrameStats();

        if (mbShowRenderer)
        {
            ImGui::Begin("Renderer", &mbShowRenderer);
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
            // pRenderer->On
            ImGui::End();
        }
    }

    void VultanaEditor::AddGUICommand(const std::string &window, const std::string &section, const std::function<void()> &command)
    {
        mCommands[window].push_back({ section, command });
    }

    void VultanaEditor::BuildDockLayout()
    {
        mDockSpace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        if (mbResetLayout)
        {
            ImGui::DockBuilderRemoveNode(mDockSpace);
            ImGui::DockBuilderAddNode(mDockSpace, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(mDockSpace, ImGui::GetMainViewport()->Size);
            mbResetLayout = false;
        }
        if (ImGui::DockBuilderGetNode(mDockSpace)->IsLeafNode())
        {
            ImGuiID left, right;
            ImGui::DockBuilderSplitNode(mDockSpace, ImGuiDir_Right, 0.2, &right, &left);
            ImGui::DockBuilderDockWindow("Renderer", right);
            ImGui::DockBuilderFinish(mDockSpace);
        }
    }

    void VultanaEditor::DrawMenu()
    {
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open Scene"))
                {
                    ifd::FileDialog::Instance().Open("SceneOpenDialog", "Open Scene", "XML file (*.xml){.xml},.*");
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug"))
            {
                if (ImGui::MenuItem("Reload Shaders"))
                {
                    pRenderer->ReloadShaders();
                }

                ImGui::EndMenu();   // End Debug Menu
            }
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("RenderGraph", ""))
                {
                    ShowRenderGraph();
                }
                ImGui::EndMenu();   // End Tools Menu
            }
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("Inspector", "", &mbShowInspector);
                ImGui::MenuItem("Settings", "", &mbShowSettings);
                ImGui::MenuItem("Renderer", "", &mbShowRenderer);
                mbResetLayout = ImGui::MenuItem("Reset Layout");

                ImGui::EndMenu();   // End Window Menu
            }
            ImGui::EndMainMenuBar();
        }

        if (ifd::FileDialog::Instance().IsDone("SceneOpenDialog"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                std::string res = ifd::FileDialog::Instance().GetResult().string();
                Core::VultanaEngine::GetEngineInstance()->GetWorld()->LoadScene(res);
            }
            ifd::FileDialog::Instance().Close();
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
        ImVec2 windowPos(ImGui::GetIO().DisplaySize.x - 200.0f, 50.0f);
        ImGuiDockNode* dockSpace = ImGui::DockBuilderGetNode(mDockSpace);
        ImGuiDockNode* centralNode = dockSpace->CentralNode;
        if (centralNode)
        {
            windowPos.x = centralNode->Size.x - 200.0f;
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 200.0f, 50.0f));
        ImGui::SetNextWindowSize(ImVec2(200.0f, 50.0f));
        ImGui::Begin("Frame Stats", nullptr, 
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | 
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    void VultanaEditor::ShowRenderGraph()
    {
        auto pEngine = Core::VultanaEngine::GetEngineInstance();
        auto pRenderer = pEngine->GetRenderer();

        std::string file = pEngine->GetWorkingPath() + "Tools/GraphViz/RenderGraph.html";
        std::string graph = pRenderer->GetRenderGraph()->Export();

        std::ofstream stream;
        stream.open(file);
        stream << R"(<!DOCTYPE html>
    <html>
      <head>
        <meta charset="utf-8">
        <title>Render Graph</title>
      </head>
      <body>
        <script src="Viz-Standalone.js"></script>
        <script>
            Viz.instance()
                .then(viz => {
                    document.body.appendChild(viz.renderSVGElement(`
    )";
        stream << graph.c_str();
        stream << R"(
                    `));
                })
                .catch(error => {
                    console.error(error);
                });
        </script>
      </body>
    </html>
    )";
    
        stream.close();

        std::string command = "start " + file;
        system(command.c_str());
    }

    void VultanaEditor::FlushPendingTextureDeletions()
    {
        for (size_t i = 0; i < mPendingDeletions.size(); i++)
        {
            RHI::RHIDescriptor* srv = mPendingDeletions[i];
            auto iter = mFileDialogIcons.find(srv);
            assert(iter != mFileDialogIcons.end());
            auto texture = iter->second;
            mFileDialogIcons.erase(srv);
            delete texture;
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