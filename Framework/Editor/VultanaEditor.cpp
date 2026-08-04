#include "VultanaEditor.hpp"
#include "Core/VultanaEngine.hpp"
#include "Editor/ImGUIImplement.hpp"
#include "Renderer/RendererBase.hpp"
#include "Utilities/Log.hpp"
#include "Utilities/String.hpp"

#include "ImFileDialog/ImFileDialog.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

namespace Editor
{
    FVultanaEditor::FVultanaEditor(Renderer::FRendererBase* pRenderer) : m_pRenderer(pRenderer)
    {
        m_pGUI = eastl::make_unique<FImGuiImplement>(pRenderer);
        m_pGUI->Init();

        ifd::FileDialog::Instance().CreateTexture = [this, pRenderer](uint8_t* data, int w, int h, char fmt) -> void*
        {
            auto pTexture = pRenderer->CreateTexture2D(w, h, 1, fmt == 1 ? RHI::ERHIFormat::RGBA8SRGB : RHI::ERHIFormat::BGRA8SRGB, 0, "ImFileDialogIcon");
            pRenderer->UploadTexture(pTexture->GetTexture(), data);

            m_FileDialogIcons.insert(eastl::make_pair(pTexture->GetSRV(), pTexture));

            return pTexture->GetSRV();
        };
        ifd::FileDialog::Instance().DeleteTexture = [this](void* tex)
        {
            m_PendingDeletions.push_back(static_cast<RHI::FRHIDescriptor*>(tex));
        };

        eastl::string assetPath = Core::FVultanaEngine::GetEngineInstance()->GetAssetsPath();
        m_pTranslateIcon.reset(pRenderer->CreateTexture2D(assetPath + "UITexture/TranslateIcon.png", true));
        m_pRotateIcon.reset(pRenderer->CreateTexture2D(assetPath + "UITexture/RotateIcon.png", true));
        m_pScaleIcon.reset(pRenderer->CreateTexture2D(assetPath + "UITexture/ScaleIcon.png", true));
    }

    FVultanaEditor::~FVultanaEditor()
    {
        for (auto iter = m_FileDialogIcons.begin(); iter != m_FileDialogIcons.end(); iter++)
        {
            delete iter->first;
            delete iter->second;
        }
    }

    void FVultanaEditor::NewFrame()
    {
        m_pGUI->NewFrame();
    }

    void FVultanaEditor::Tick()
    {
        FlushPendingTextureDeletions();

        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse && io.MouseClicked[0])
        {
            ImVec2 mousePos = io.MouseClickedPos[0];
            m_pRenderer->RequestMouseHitTest((uint32_t)mousePos.x, (uint32_t)mousePos.y);
        }

        BuildDockLayout();
        DrawMenu();
        DrawToolBar();
        DrawGizmo();
        DrawFrameStats();

        if (m_bShowRenderer)
        {
            ImGui::Begin("Renderer", &m_bShowRenderer);
            // m_pRenderer->OnGUI();
            ImGui::End();
        }
        if (m_bShowWorldOutliner)
        {
            ImGui::Begin("WorldOutliner", &m_bShowWorldOutliner);
            auto pWorld = Core::FVultanaEngine::GetEngineInstance()->GetWorld();
            pWorld->OnGUI();
            ImGui::End();
        }
        if (m_bShowGPUDrivenStats)
        {
            m_pRenderer->GetGPUDrivenStats()->OnGui();
        }
    }

    void FVultanaEditor::Render(RHI::FRHICommandList *pCmdList)
    {
        if (m_bShowInspector)
        {
            DrawWindow("Inspector", &m_bShowInspector);
        }
        if (m_bShowSettings)
        {
            DrawWindow("Settings", &m_bShowSettings);
        }
        m_pGUI->Render(pCmdList);
        m_Commands.clear();
    }

    void FVultanaEditor::AddGUICommand(const eastl::string &window, const eastl::string &section, const eastl::function<void()> &command)
    {
        m_Commands[window].push_back({ section, command });
    }

    void FVultanaEditor::BuildDockLayout()
    {
        m_DockSpace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_bResetLayout)
        {
            ImGui::DockBuilderRemoveNode(m_DockSpace);
            ImGui::DockBuilderAddNode(m_DockSpace, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(m_DockSpace, ImGui::GetMainViewport()->WorkSize);
            m_bResetLayout = false;
        }
        if (ImGui::DockBuilderGetNode(m_DockSpace)->IsLeafNode())
        {
            ImGuiID left, right;
            ImGui::DockBuilderSplitNode(m_DockSpace, ImGuiDir_Right, 0.2, &right, &left);
            ImGui::DockBuilderDockWindow("Renderer", left);
            ImGui::DockBuilderDockWindow("WorldOutliner", right);
            ImGui::DockBuilderFinish(m_DockSpace);
        }
    }

    void FVultanaEditor::DrawToolBar()
    {
        ImGui::SetNextWindowPos(ImVec2(200, 20));
        ImGui::SetNextWindowSize(ImVec2(300, 30));

        ImGui::Begin("EditorToolBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

        ImVec4 focusedBG(1.0f, 0.6f, 0.2f, 0.5f);
        ImVec4 normalBG(0.0f, 0.0f, 0.0f, 0.0f);

        if (ImGui::ImageButton("translate_button##editor", (ImTextureID)m_pTranslateIcon->GetSRV(), ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), m_SelectEditMode == ESelectEditMode::Translate ? focusedBG : normalBG) ||
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W), false))
        {
            m_SelectEditMode = ESelectEditMode::Translate;
        }

        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::ImageButton("rotate_button##editor", (ImTextureID)m_pRotateIcon->GetSRV(), ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), m_SelectEditMode == ESelectEditMode::Rotate ? focusedBG : normalBG) ||
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_E), false))
        {
            m_SelectEditMode = ESelectEditMode::Rotate;
        }

        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::ImageButton("scale_button##editor", (ImTextureID)m_pScaleIcon->GetSRV(), ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), m_SelectEditMode == ESelectEditMode::Scale ? focusedBG : normalBG) ||
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_R), false))
        {
            m_SelectEditMode = ESelectEditMode::Scale;
        }
        ImGui::PopStyleVar();

        ImGui::End();
    }

    void FVultanaEditor::DrawMenu()
    {
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
                if (ImGui::MenuItem("GPU Driven Stats", "", &m_bShowGPUDrivenStats))
                {
                    m_pRenderer->SetGPUDrivenStatsEnabled(m_bShowGPUDrivenStats);
                }

                if (ImGui::MenuItem("Show Meshlets", "", &m_bShowMeshlets))
                {
                    m_pRenderer->SetShowMeshletsEnabled(m_bShowMeshlets);
                }

                if (ImGui::MenuItem("VSync", "", &m_bVSync))
                {
                    m_pRenderer->GetSwapchain()->SetVSyncEnabled(m_bVSync);
                }

                if (ImGui::MenuItem("Reload Shaders"))
                {
                    m_pRenderer->ReloadShaders();
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
                ImGui::MenuItem("Inspector", "", &m_bShowInspector);
                ImGui::MenuItem("Settings", "", &m_bShowSettings);
                ImGui::MenuItem("Renderer", "", &m_bShowRenderer);
                ImGui::MenuItem("WorldOutliner", "", &m_bShowWorldOutliner);
                m_bResetLayout = ImGui::MenuItem("Reset Layout");

                ImGui::EndMenu();   // End Window Menu
            }
            ImGui::EndMainMenuBar();
        }

        if (ifd::FileDialog::Instance().IsDone("SceneOpenDialog"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                eastl::string res = ifd::FileDialog::Instance().GetResult().string().c_str();
                Core::FVultanaEngine::GetEngineInstance()->GetWorld()->LoadScene(res);
            }
            ifd::FileDialog::Instance().Close();
        }

        if (m_bShowImGuiDemo)
        {
            ImGui::ShowDemoWindow(&m_bShowImGuiDemo);
        }
    }

    void FVultanaEditor::DrawGizmo()
    {
        auto pWorld = Core::FVultanaEngine::GetEngineInstance()->GetWorld();

        auto pSelectedObject = pWorld->GetVisibleObject(m_pRenderer->GetMouseHitObjectID());
        if (pSelectedObject == nullptr) return;

        float3 position = pSelectedObject->GetPosition();
        float3 rotation = RotationAngles(pSelectedObject->GetRotation());

        float3 scale = pSelectedObject->GetScale();

        float4x4 mtxWorld;
        ImGuizmo::RecomposeMatrixFromComponents((const float*)&position, (const float*)&rotation, (const float*)&scale, (float*)&mtxWorld);

        ImGuizmo::OPERATION operation;
        switch (m_SelectEditMode)
        {
        case ESelectEditMode::Translate:
            operation = ImGuizmo::TRANSLATE;
            break;
        case ESelectEditMode::Rotate:
            operation = ImGuizmo::ROTATE;
            break;
        case ESelectEditMode::Scale:
            operation = ImGuizmo::SCALE;
            break;
        default:
            assert(false);
            break;
        }

        Scene::FCamera* pCamera = pWorld->GetCamera();
        float4x4 view = pCamera->GetViewMatrix();
        float4x4 proj = pCamera->GetProjectionMatrix();

        ImGuizmo::AllowAxisFlip(false);
        ImGuizmo::Manipulate((const float*)&view, (const float*)&proj, operation, ImGuizmo::WORLD, (float*)&mtxWorld);

        ImGuizmo::DecomposeMatrixToComponents((const float*)&mtxWorld, (float*)&position, (float*)&rotation, (float*)&scale);

        pSelectedObject->SetPosition(position);
        pSelectedObject->SetRotation(RotationQuat(rotation));
        pSelectedObject->SetScale(scale);
        
        pSelectedObject->OnGUI();
    }

    void FVultanaEditor::DrawFrameStats()
    {
        ImVec2 windowPos(ImGui::GetIO().DisplaySize.x - 200.0f, 50.0f);
        ImGuiDockNode* dockSpace = ImGui::DockBuilderGetNode(m_DockSpace);
        ImGuiDockNode* centralNode = dockSpace->CentralNode;
        if (centralNode)
        {
            windowPos.x = centralNode->Size.x - 200.0f;
        }

        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(ImVec2(200.0f, 50.0f));
        ImGui::Begin("Frame Stats", nullptr, 
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | 
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    void FVultanaEditor::ShowRenderGraph()
    {
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();

        eastl::string file = pEngine->GetWorkingPath() + "Tools/GraphViz/RenderGraph.html";
        eastl::string graph = m_pRenderer->GetRenderGraph()->Export();

        std::ofstream stream;
        stream.open(file.c_str());
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

        eastl::string command = "start " + file;
        system(command.c_str());
    }

    void FVultanaEditor::FlushPendingTextureDeletions()
    {
        for (size_t i = 0; i < m_PendingDeletions.size(); i++)
        {
            RHI::FRHIDescriptor* srv = m_PendingDeletions[i];
            auto iter = m_FileDialogIcons.find(srv);
            assert(iter != m_FileDialogIcons.end());
            auto texture = iter->second;
            m_FileDialogIcons.erase(srv);
            delete texture;
        }
        m_PendingDeletions.clear();
    }

    void FVultanaEditor::DrawWindow(const eastl::string &window, bool *pOpen)
    {
        ImGui::Begin(window.c_str(), pOpen);

        auto iter = m_Commands.find(window);
        if (iter != m_Commands.end())
        {
            for (size_t i = 0; i < iter->second.size(); i++)
            {
                const FCommand& cmd = iter->second[i];
                if (ImGui::CollapsingHeader(cmd.Section.c_str()))
                {
                    cmd.Function();
                }
            }
        }
        ImGui::End();
    }
}