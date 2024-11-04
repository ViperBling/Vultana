#include "VultanaEditor.hpp"
#include "Core/VultanaEngine.hpp"
#include "Core/VultanaGUI.hpp"
#include "Renderer/RendererBase.hpp"
#include "Utilities/Log.hpp"
#include "Utilities/String.hpp"

#include "ImFileDialog/ImFileDialog.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

namespace Core
{
    VultanaEditor::VultanaEditor()
    {
        ifd::FileDialog::Instance().CreateTexture = [this](uint8_t* data, int w, int h, char fmt) -> void*
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
            auto pTexture = pRenderer->CreateTexture2D(w, h, 1, fmt == 1 ? RHI::ERHIFormat::RGBA8SRGB : RHI::ERHIFormat::BGRA8SRGB, 0, "ImFileDialogIcon");
            pRenderer->UploadTexture(pTexture->GetTexture(), data);

            mFileDialogIcons.insert(eastl::make_pair(pTexture->GetSRV(), pTexture));

            return pTexture->GetSRV();
        };
        ifd::FileDialog::Instance().DeleteTexture = [this](void* tex)
        {
            mPendingDeletions.push_back(static_cast<RHI::RHIDescriptor*>(tex));
        };

        eastl::string assetPath = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath();
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        mpTranslateIcon.reset(pRenderer->CreateTexture2D(assetPath + "UITexture/TranslateIcon.png", true));
        mpRotateIcon.reset(pRenderer->CreateTexture2D(assetPath + "UITexture/RotateIcon.png", true));
        mpScaleIcon.reset(pRenderer->CreateTexture2D(assetPath + "UITexture/ScaleIcon.png", true));
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

        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse && io.MouseClicked[0])
        {
            ImVec2 mousePos = io.MouseClickedPos[0];

            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
            pRenderer->RequestMouseHitTest((uint32_t)mousePos.x, (uint32_t)mousePos.y);
        }

        BuildDockLayout();
        DrawMenu();
        DrawToolBar();
        DrawGizmo();
        DrawFrameStats();

        if (mbShowRenderer)
        {
            ImGui::Begin("Renderer", &mbShowRenderer);
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
            // pRenderer->OnG
            ImGui::End();
        }
        if (mbShowWorldOutliner)
        {
            ImGui::Begin("WorldOutliner", &mbShowWorldOutliner);
            auto pWorld = Core::VultanaEngine::GetEngineInstance()->GetWorld();
            pWorld->OnGUI();
            ImGui::End();
        }
    }

    void VultanaEditor::AddGUICommand(const eastl::string &window, const eastl::string &section, const eastl::function<void()> &command)
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
            ImGui::DockBuilderSetNodeSize(mDockSpace, ImGui::GetMainViewport()->WorkSize);
            mbResetLayout = false;
        }
        if (ImGui::DockBuilderGetNode(mDockSpace)->IsLeafNode())
        {
            ImGuiID left, right;
            ImGui::DockBuilderSplitNode(mDockSpace, ImGuiDir_Right, 0.2, &right, &left);
            ImGui::DockBuilderDockWindow("Renderer", left);
            ImGui::DockBuilderDockWindow("WorldOutliner", right);
            ImGui::DockBuilderFinish(mDockSpace);
        }
    }

    void VultanaEditor::DrawToolBar()
    {
        ImGui::SetNextWindowPos(ImVec2(200, 20));
        ImGui::SetNextWindowSize(ImVec2(300, 30));

        ImGui::Begin("EditorToolBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

        ImVec4 focusedBG(1.0f, 0.6f, 0.2f, 0.5f);
        ImVec4 normalBG(0.0f, 0.0f, 0.0f, 0.0f);

        if (ImGui::ImageButton("translate_button##editor", (ImTextureID)mpTranslateIcon->GetSRV(), ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), mSelectEditMode == ESelectEditMode::Translate ? focusedBG : normalBG) ||
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W), false))
        {
            mSelectEditMode = ESelectEditMode::Translate;
        }

        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::ImageButton("rotate_button##editor", (ImTextureID)mpRotateIcon->GetSRV(), ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), mSelectEditMode == ESelectEditMode::Rotate ? focusedBG : normalBG) ||
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_E), false))
        {
            mSelectEditMode = ESelectEditMode::Rotate;
        }

        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::ImageButton("scale_button##editor", (ImTextureID)mpScaleIcon->GetSRV(), ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), mSelectEditMode == ESelectEditMode::Scale ? focusedBG : normalBG) ||
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_R), false))
        {
            mSelectEditMode = ESelectEditMode::Scale;
        }
        ImGui::PopStyleVar();

        ImGui::End();
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
                ImGui::MenuItem("WorldOutliner", "", &mbShowWorldOutliner);
                mbResetLayout = ImGui::MenuItem("Reset Layout");

                ImGui::EndMenu();   // End Window Menu
            }
            ImGui::EndMainMenuBar();
        }

        if (ifd::FileDialog::Instance().IsDone("SceneOpenDialog"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                eastl::string res = ifd::FileDialog::Instance().GetResult().string().c_str();
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

    void VultanaEditor::DrawGizmo()
    {
        auto pWorld = Core::VultanaEngine::GetEngineInstance()->GetWorld();
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

        auto pSelectedObject = pWorld->GetVisibleObject(pRenderer->GetMouseHitObjectID());
        if (pSelectedObject == nullptr) return;

        float3 position = pSelectedObject->GetPosition();
        float3 rotation = RotationAngles(pSelectedObject->GetRotation());

        float3 scale = pSelectedObject->GetScale();

        float4x4 mtxWorld;
        ImGuizmo::RecomposeMatrixFromComponents((const float*)&position, (const float*)&rotation, (const float*)&scale, (float*)&mtxWorld);

        ImGuizmo::OPERATION operation;
        switch (mSelectEditMode)
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

        Scene::Camera* pCamera = pWorld->GetCamera();
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

    void VultanaEditor::DrawFrameStats()
    {
        ImVec2 windowPos(ImGui::GetIO().DisplaySize.x - 200.0f, 50.0f);
        ImGuiDockNode* dockSpace = ImGui::DockBuilderGetNode(mDockSpace);
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

    void VultanaEditor::ShowRenderGraph()
    {
        auto pEngine = Core::VultanaEngine::GetEngineInstance();
        auto pRenderer = pEngine->GetRenderer();

        eastl::string file = pEngine->GetWorkingPath() + "Tools/GraphViz/RenderGraph.html";
        eastl::string graph = pRenderer->GetRenderGraph()->Export();

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

    void VultanaEditor::DrawWindow(const eastl::string &window, bool *pOpen)
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