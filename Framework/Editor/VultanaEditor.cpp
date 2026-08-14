#include "Editor/VultanaEditor.hpp"
#include "AssetManager/ModelLoaderECS.hpp"
#include "AssetManager/SceneAssetRegistry.hpp"
#include "Core/VultanaEngine.hpp"

#include "Editor/Commands/EntityEditableTarget.hpp"
#include "Editor/Commands/NameCommand.hpp"
#include "Editor/Commands/ObjectEditableTarget.hpp"
#include "Editor/Commands/ParentCommand.hpp"
#include "Editor/ImGUIImplement.hpp"

#include "Renderer/RendererBase.hpp"
#include "Scene/ECS/Components.hpp"
#include "Scene/ECS/SceneSerializer.hpp"

#include "Utilities/Log.hpp"
#include "Utilities/String.hpp"

#include <EASTL/memory.h>

#include "ImFileDialog/ImFileDialog.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

#include <SimpleIni.h>

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
        auto pEngineInst = Core::FVultanaEngine::GetEngineInstance();
        if (!io.WantCaptureMouse && io.MouseClicked[0])
        {
            ImVec2 mousePos = io.MouseClickedPos[0];
            m_pRenderer->RequestMouseHitTest((uint32_t)mousePos.x, (uint32_t)mousePos.y);
        }

        if (pEngineInst->IsUseECSBackend())
        {
            const uint32_t mouseID = m_pRenderer->GetMouseHitObjectID();
            if (mouseID != m_LastMouseHitID)
            {
                m_LastMouseHitID = mouseID;
                if (mouseID != UINT32_MAX)
                {
                    auto* pEntitySystem = pEngineInst->GetEntitySystem();
                    const entt::entity picked = pEntitySystem->ResolvePickID(mouseID);
                    if (picked != entt::null)
                        SelectEntity(picked);
                }
            }
        }

        // Undo/Redo Shortcut: Ctrl + Z / Ctrl + Y / Ctrl + Shift + Z
        if (!io.WantCaptureKeyboard && !m_bGizmoDragging)
        {
            const bool ctrl = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
            {
                const bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
                if (shift)
                {
                    m_EditHistory.Redo();
                }
                else
                {
                    m_EditHistory.Undo();
                }
            }
            else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y))
            {
                m_EditHistory.Redo();
            }
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
            if (pEngineInst->IsUseECSBackend())
            {
                DrawOutlinerECS();
            }
            else
            {
                auto pWorld = pEngineInst->GetWorld();
                pWorld->OnGUI();
            }
            ImGui::End();
        }
        if (m_bShowGPUDrivenStats)
        {
            m_pRenderer->GetGPUDrivenStats()->OnGui();
        }
        if (m_bShowAssetManager)
        {
            DrawAssetManagerPanel();
        }
    }

    void FVultanaEditor::Render(RHI::FRHICommandList *pCmdList)
    {
        if (m_bShowInspector)
        {
            AddGUICommand("Inspector", "Entity",
            [this]()
            { 
                DrawInspectorECS(); 
            });
            AddGUICommand("Inspector", "Selected Model",
            [this]()
            { 
                DrawInspectorModelSection(); 
            });
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
                if (ImGui::MenuItem("Save Scene"))
                {
                    SaveScene();
                }
                if (ImGui::MenuItem("Open Model"))
                {
                    eastl::string modelsDir = Core::FVultanaEngine::GetEngineInstance()->GetAssetsPath() + "Models";
                    ifd::FileDialog::Instance().Open("ModelOpenDialog", "Open Model",  "Model files (*.gltf;*.glb;*.fbx;*.obj;*.dae){.gltf,.glb,.fbx,.obj,.dae},.*",  /*isMultiselect=*/false, modelsDir.c_str());
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_EditHistory.GetUndoCount() > 0))
                {
                    m_EditHistory.Undo();
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_EditHistory.GetRedoCount() > 0))
                {
                    m_EditHistory.Redo();
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
                if (ImGui::MenuItem("Reload Models"))
                {
                    ReloadAllModels();
                }
                if (ImGui::MenuItem("Reload Textures"))
                {
                    ReloadAllTextures();
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
                ImGui::MenuItem("AssetManager", "", &m_bShowAssetManager);
                m_bResetLayout = ImGui::MenuItem("Reset Layout");

                ImGui::EndMenu();   // End Window Menu
            }
            ImGui::EndMainMenuBar();
        }

        if (ifd::FileDialog::Instance().IsDone("SceneOpenDialog"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                const eastl::string res = ifd::FileDialog::Instance().GetResult().string().c_str();
                OpenScene(res);
            }
            ifd::FileDialog::Instance().Close();
        }
        if (ifd::FileDialog::Instance().IsDone("ModelOpenDialog"))
        {
            if (ifd::FileDialog::Instance().HasResult())
            {
                eastl::string res = ifd::FileDialog::Instance().GetResult().string().c_str();
                Assets::FModelLoaderECS::LoadModelAtCamera(res);
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
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        if (pEngine->IsUseECSBackend())
        {
            DrawGizmoECS();
            return;
        }
        auto pWorld = pEngine->GetWorld();

        auto pSelectedObject = pWorld->GetVisibleObject(m_pRenderer->GetMouseHitObjectID());
        if (pSelectedObject == nullptr)
        {
            m_bGizmoDragging = false;
            m_GizmoDragTarget = nullptr;
            return;
        }

        const bool usingGizmo = ImGuizmo::IsUsing();
        if (usingGizmo && !m_bGizmoDragging)
        {
            m_bGizmoDragging = true;
            m_GizmoDragTarget = pSelectedObject;
            m_GizmoDragStart = Editor::Commands::FObjectEditableTarget(*pSelectedObject).GetTransform();
        }

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

        if (m_bGizmoDragging && !usingGizmo)
        {
            // Dragging ended → push command to history
            m_bGizmoDragging = false;
            if (m_GizmoDragTarget == pSelectedObject)
            {
                Editor::Commands::FObjectEditableTarget finalTarget(*m_GizmoDragTarget);
                // Click handle but no change → do not push command
                if (!Editor::Commands::TransformEqual(m_GizmoDragStart, finalTarget.GetTransform()))
                {
                    m_EditHistory.Push(eastl::make_unique<Editor::Commands::FSetTransformCommand>(
                        eastl::make_unique<Editor::Commands::FObjectEditableTarget>(*m_GizmoDragTarget),
                        m_GizmoDragStart,
                        finalTarget.GetTransform()));
                }
            }
            m_GizmoDragTarget = nullptr;
        }
    }

    void FVultanaEditor::DrawGizmoECS()
    {
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pEntitySystem = pEngine->GetEntitySystem();
        auto* pWorld = pEngine->GetWorld();
        if (pEntitySystem == nullptr || pWorld == nullptr) return;

        auto& reg = pEntitySystem->GetRegistry();
        const entt::entity target = (m_SelectedEntity == entt::null) ? entt::null : Assets::FModelLoaderECS::FindModelInstanceRoot(reg, m_SelectedEntity);

        if (target == entt::null || !reg.valid(target) || reg.try_get<Scene::ECS::FTransformComponent>(target) == nullptr)
        {
            m_bGizmoDragging = false;
            m_GizmoDragEntity = entt::null;
            return;
        }

        Editor::Commands::FEntityEditableTarget editTarget(*pEntitySystem, target);

        const bool usingGizmo = ImGuizmo::IsUsing();
        if (usingGizmo && !m_bGizmoDragging)
        {
            m_bGizmoDragging = true;
            m_GizmoDragEntity = target;
            m_GizmoDragStart = editTarget.GetTransform();
        }

        Editor::Commands::FTransformSnapshot current = editTarget.GetTransform();
        float3 position = current.Position;
        float3 rotation = RotationAngles(current.Rotation);
        float3 scale = current.Scale;

        float4x4 mtxWorld;
        ImGuizmo::RecomposeMatrixFromComponents((const float*)&position, (const float*)&rotation, (const float*)&scale, (float*)&mtxWorld);

        ImGuizmo::OPERATION operation;
        switch (m_SelectEditMode)
        {
        case ESelectEditMode::Translate:    operation = ImGuizmo::TRANSLATE; break;
        case ESelectEditMode::Rotate:       operation = ImGuizmo::ROTATE; break;
        case ESelectEditMode::Scale:        operation = ImGuizmo::SCALE; break;
        default: assert(false);             operation = ImGuizmo::TRANSLATE; break;
        }

        Scene::FCamera* pCamera = pWorld->GetCamera();
        float4x4 view = pCamera->GetViewMatrix();
        float4x4 proj = pCamera->GetProjectionMatrix();

        ImGuizmo::AllowAxisFlip(false);
        const bool bChanged = ImGuizmo::Manipulate((const float*)&view, (const float*)&proj, operation, ImGuizmo::WORLD, (float*)&mtxWorld);
        if (bChanged)
        {
            ImGuizmo::DecomposeMatrixToComponents((const float*)&mtxWorld, (float*)&position, (float*)&rotation, (float*)&scale);

            Editor::Commands::FTransformSnapshot next;
            next.Position = position;
            next.Rotation = RotationQuat(rotation);
            next.Scale = scale;
            editTarget.SetTransform(next);
        }

        if (m_bGizmoDragging && !usingGizmo)
        {
            m_bGizmoDragging = false;
            if (m_GizmoDragEntity == target)
            {
                const Editor::Commands::FTransformSnapshot finalTF = editTarget.GetTransform();
                if (!Editor::Commands::TransformEqual(m_GizmoDragStart, finalTF))
                {
                    m_EditHistory.Push(eastl::make_unique<Editor::Commands::FSetTransformCommand>(
                        eastl::make_unique<Editor::Commands::FEntityEditableTarget>(*pEntitySystem, target),
                        m_GizmoDragStart,
                        finalTF));
                }
            }
            m_GizmoDragEntity = entt::null;
        }
    }

    void FVultanaEditor::DrawOutlinerECS()
    {
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pEntitySystem = pEngine->GetEntitySystem();
        if (pEntitySystem == nullptr)
            return;
        auto& reg = pEntitySystem->GetRegistry();
        auto& childrenMap = pEntitySystem->GetChildrenMap();

        eastl::function<void(entt::entity)> drawNode;
        drawNode = [&](entt::entity entity)
        {
            const char* name = "Entity";
            if (const auto* nameComp = reg.try_get<Scene::ECS::FNameComponent>(entity))
                name = nameComp->Name;

            ImGui::PushID(static_cast<int>(entt::to_integral(entity)));
            const bool bSelected = (entity == m_SelectedEntity);
            const bool bLeaf = childrenMap.find(entt::to_integral(entity)) == childrenMap.end();
            const bool bOpen = ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_OpenOnArrow | (bSelected ? ImGuiTreeNodeFlags_Selected : 0) | (bLeaf ? ImGuiTreeNodeFlags_Leaf : 0));
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                SelectEntity(entity);

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("ECS_ENTITY", &entity, sizeof(entity));
                ImGui::Text("Reparent: %s", name);
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ECS_ENTITY"))
                {
                    entt::entity src = *static_cast<const entt::entity*>(payload->Data);
                    if (src != entity && src != entt::null)
                    {
                        const auto* parentComp = reg.try_get<Scene::ECS::FParentComponent>(src);
                        const entt::entity before = parentComp ? parentComp->Parent : entt::null;
                        m_EditHistory.Push(eastl::make_unique<Editor::Commands::FSetParentCommand>(
                            *pEntitySystem, src, before, entity));
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (bOpen)
            {
                if (const auto it = childrenMap.find(entt::to_integral(entity)); it != childrenMap.end())
                {
                    for (entt::entity child : it->second)
                        drawNode(child);
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        };

        auto rootView = reg.view<Scene::ECS::FTransformComponent>(entt::exclude_t<Scene::ECS::FParentComponent>());
        for (entt::entity root : rootView)
            drawNode(root);
        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_SelectedEntity = entt::null;
            m_pRenderer->SetSelectedObjectID(UINT32_MAX);
            m_LastMouseHitID = UINT32_MAX;   // so clicking the same object re-selects it
        }
    }

    void FVultanaEditor::DrawInspectorECS()
    {
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pEntitySystem = pEngine->GetEntitySystem();
        if (pEntitySystem == nullptr)
            return;
        if (m_SelectedEntity == entt::null || !pEntitySystem->GetRegistry().valid(m_SelectedEntity))
        {
            ImGui::TextDisabled("No entity selected (click in Outliner or viewport).");
            return;
        }
        DrawTransformEditorECS(*pEntitySystem, m_SelectedEntity);

        auto& reg = pEntitySystem->GetRegistry();
        if (const auto* model = reg.try_get<Scene::ECS::FModelInstanceComponent>(m_SelectedEntity))
        {
            ImGui::Separator();
            ImGui::TextWrapped("Model: %s", model->ModelPath.c_str());
        }
        if (reg.all_of<Scene::ECS::FMainCameraTag>(m_SelectedEntity))
        {
            ImGui::Separator();
            ImGui::TextDisabled("Camera entity");
        }
        if (reg.all_of<Scene::ECS::FMainLightTag>(m_SelectedEntity))
        {
            ImGui::Separator();
            ImGui::TextDisabled("Light entity");
        }
    }

    void FVultanaEditor::DrawTransformEditorECS(Scene::ECS::FEntitySubsystem& entitySubsystem, entt::entity entity)
    {
        Editor::Commands::FEntityEditableTarget editTarget(entitySubsystem, entity);
        const Editor::Commands::FTransformSnapshot current = editTarget.GetTransform();

        if (auto* nameComp = entitySubsystem.GetRegistry().try_get<Scene::ECS::FNameComponent>(entity))
        {
            char nameBuf[64];
            strncpy_s(nameBuf, nameComp->Name, _TRUNCATE);
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                const eastl::string beforeName(nameComp->Name);
                nameComp->SetName(nameBuf);
                m_EditHistory.Push(eastl::make_unique<Editor::Commands::FSetNameCommand>(entitySubsystem, entity, beforeName.c_str(), nameBuf));
            }
        }

        float3 p = current.Position;
        float3 r = RotationAngles(current.Rotation);
        float3 s = current.Scale;

        bool bChanged = false;
        bool bActivated = false;
        bool bReleased = false;
        bChanged    |= ImGui::DragFloat3("Position", (float*)&p, 0.1f);
        bActivated  |= ImGui::IsItemActivated();
        bReleased   |= ImGui::IsItemDeactivatedAfterEdit();
        bChanged    |= ImGui::DragFloat3("Rotation", (float*)&r, 0.5f);
        bActivated  |= ImGui::IsItemActivated();
        bReleased   |= ImGui::IsItemDeactivatedAfterEdit();
        bChanged    |= ImGui::DragFloat3("Scale", (float*)&s, 0.01f);
        bActivated  |= ImGui::IsItemActivated();
        bReleased   |= ImGui::IsItemDeactivatedAfterEdit();

        if (bActivated && !m_bInspectorDragging)
        {
            m_bInspectorDragging = true;
            m_InspectorDragStart = current; // sampled at drag start, not per frame
        }
        if (bChanged)
        {
            Editor::Commands::FTransformSnapshot next;
            next.Position = p;
            next.Rotation = RotationQuat(r);
            next.Scale = s;
            editTarget.SetTransform(next); // marks transform dirty → this frame recompute
        }
        if (bReleased && m_bInspectorDragging)
        {
            m_bInspectorDragging = false;
            const Editor::Commands::FTransformSnapshot finalTF = editTarget.GetTransform();
            if (!Editor::Commands::TransformEqual(m_InspectorDragStart, finalTF))
            {
                m_EditHistory.Push(eastl::make_unique<Editor::Commands::FSetTransformCommand>(eastl::make_unique<Editor::Commands::FEntityEditableTarget>(entitySubsystem, entity), m_InspectorDragStart, finalTF));
            }
        }
    }

    void FVultanaEditor::SaveScene()
    {
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        if (!pEngine->IsUseECSBackend())
        {
            VTNA_LOG_WARN("[Editor] Save Scene requires the ECS scene backend");
            return;
        }
        auto* pEntitySystem = pEngine->GetEntitySystem();
        if (pEntitySystem == nullptr)
            return;

        if (pEngine->GetAssetManager() && pEngine->GetAssetManager()->HasPendingRequests())
        {
            VTNA_LOG_WARN("[Editor] Save Scene: assets still loading; try again when idle");
            return;
        }

        const bool ok = Scene::ECS::FSceneSerializer::Save(*pEntitySystem, pEngine->GetSceneFilePath(), pEngine->GetAssetsPath());
        if (ok)
            VTNA_LOG_INFO("[Editor] Scene saved: {}", pEngine->GetSceneFilePath());
        else
            VTNA_LOG_ERROR("[Editor] Scene save failed: {}", pEngine->GetSceneFilePath());
    }

    void FVultanaEditor::OpenScene(const eastl::string& path)
    {
        Core::FVultanaEngine::GetEngineInstance()->LoadSceneFromFile(path);
    }

    void FVultanaEditor::SelectEntity(entt::entity entity)
    {
        m_SelectedEntity = entity;
        auto* pEntitySystem = Core::FVultanaEngine::GetEngineInstance()->GetEntitySystem();

        entt::entity highlight = entity;
        auto& reg = pEntitySystem->GetRegistry();
        if (!reg.all_of<Scene::ECS::FMeshRendererComponent>(highlight))
        {
            eastl::vector<entt::entity> stack;
            if (const auto* children = pEntitySystem->FindChildren(entity))
                stack = *children;
            while (!stack.empty())
            {
                const entt::entity c = stack.back();
                stack.pop_back();
                if (reg.all_of<Scene::ECS::FMeshRendererComponent>(c))
                {
                    highlight = c;
                    break;
                }
                if (const auto* ch = pEntitySystem->FindChildren(c))
                    stack.insert(stack.end(), ch->begin(), ch->end());
            }
        }
        m_pRenderer->SetSelectedObjectID(pEntitySystem->GetPickID(highlight));
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

    void FVultanaEditor::ReloadAllModels()
    {
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        if (!pEngine->IsUseECSBackend())
        {
            VTNA_LOG_WARN("[Editor] Reload Models requires the ECS scene backend (Config: World/SceneBackend = ECS)");
            return;
        }

        auto* pAssetManager = pEngine->GetAssetManager();
        if (pAssetManager && pAssetManager->HasPendingRequests())
        {
            VTNA_LOG_WARN("[Editor] Reload Models: assets still loading; try again when idle");
            return;
        }
        auto* pAssetRegistry = static_cast<Assets::FSceneAssetRegistry*>(pEngine->GetAssetRegistry());
        if (pAssetRegistry == nullptr)
            return;
        eastl::vector<Assets::FSceneAssetRegistry::FModelStatus> models;
        pAssetRegistry->EnumerateModels(models);
        for (const auto& m : models)
            Assets::FModelLoaderECS::ReloadModelAsync(m.ModelPath);
        VTNA_LOG_INFO("[Editor] Reload Models: {} model(s) reloading", models.size());
    }

    void FVultanaEditor::ReloadAllTextures()
    {
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        if (!pEngine->IsUseECSBackend())
        {
            VTNA_LOG_WARN("[Editor] Reload Textures requires the ECS scene backend (Config: World/SceneBackend = ECS)");
            return;
        }
        auto* pAssetManager = pEngine->GetAssetManager();
        if (pAssetManager && pAssetManager->HasPendingRequests())
        {
            VTNA_LOG_WARN("[Editor] Reload Textures: assets still loading; try again when idle");
            return;
        }
        auto* pAssetRegistry = static_cast<Assets::FSceneAssetRegistry*>(pEngine->GetAssetRegistry());
        if (pAssetRegistry == nullptr)
            return;

        eastl::vector<Assets::FSceneAssetRegistry::FTextureStatus> textures;
        pAssetRegistry->EnumerateTextures(textures);
        uint32_t staleCount = 0;
        for (const auto& t : textures)
        {
            if (!pAssetManager->IsTextureProductStale(t.ProductPath))
                continue;
            Assets::FModelLoaderECS::ReloadTextureAsync(t.ProductPath);
            ++staleCount;
        }
        VTNA_LOG_INFO("[Editor] Reload Textures: {}/{} texture(s) stale, reloading", staleCount, (uint32_t)textures.size());
    }

    void FVultanaEditor::DrawAssetManagerPanel()
    {
        if (!ImGui::Begin("Asset Manager", &m_bShowAssetManager))
        {
            ImGui::End();
            return;
        }

        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pAssetManager = pEngine->GetAssetManager();
        auto* pAssetRegistry = static_cast<Assets::FSceneAssetRegistry*>(pEngine->GetAssetRegistry());
        if (pAssetManager)
        {
            ImGui::Text("Pending requests: %u", pAssetManager->GetPendingRequestCount());
        }
        ImGui::Separator();

        if (pAssetRegistry)
        {
            static const char* kStateNames[] = {"Pending", "Loading", "Ready", "Failed"};
            eastl::vector<Assets::FSceneAssetRegistry::FModelStatus> models;
            pAssetRegistry->EnumerateModels(models);
            if (ImGui::BeginTable("##asset_models", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("Model");
                ImGui::TableSetupColumn("State");
                ImGui::TableSetupColumn("Meshes");
                ImGui::TableSetupColumn("Prims");
                ImGui::TableSetupColumn("Action");
                ImGui::TableHeadersRow();
                const bool bBusy = pAssetManager && pAssetManager->HasPendingRequests();
                for (size_t row = 0; row < models.size(); ++row)
                {
                    const auto& m = models[row];
                    ImGui::PushID(static_cast<int>(row));
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(m.ModelPath.c_str());
                    ImGui::TableNextColumn();
                    const uint32_t state = static_cast<uint32_t>(m.State);
                    ImGui::TextUnformatted(state < 4 ? kStateNames[state] : "?");
                    ImGui::TableNextColumn();
                    ImGui::Text("%u", m.MeshCount);
                    ImGui::TableNextColumn();
                    ImGui::Text("%u", m.PrimitiveCount);
                    ImGui::TableNextColumn();
                    ImGui::BeginDisabled(bBusy);
                    if (ImGui::SmallButton("Reload"))
                    {
                        Assets::FModelLoaderECS::ReloadModelAsync(m.ModelPath);
                    }
                    ImGui::EndDisabled();
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }

            ImGui::Separator();
            ImGui::Text("Textures");
            eastl::vector<Assets::FSceneAssetRegistry::FTextureStatus> textures;
            pAssetRegistry->EnumerateTextures(textures);
            if (ImGui::BeginTable("##asset_textures", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("Texture");
                ImGui::TableSetupColumn("GPU");
                ImGui::TableSetupColumn("Action");
                ImGui::TableHeadersRow();
                const bool bBusy = pAssetManager && pAssetManager->HasPendingRequests();
                for (size_t row = 0; row < textures.size(); ++row)
                {
                    const auto& t = textures[row];
                    ImGui::PushID(static_cast<int>(row));
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(t.ProductPath.c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(t.bResident ? "Ready" : "-");
                    ImGui::TableNextColumn();
                    ImGui::BeginDisabled(bBusy);
                    if (ImGui::SmallButton("Reload"))
                    {
                        Assets::FModelLoaderECS::ReloadTextureAsync(t.ProductPath);
                    }
                    ImGui::EndDisabled();
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::Text("No asset registry (ECS backend disabled)");
        }

        ImGui::End();
    }

    void FVultanaEditor::DrawInspectorModelSection()
    {
        auto pEngine = Core::FVultanaEngine::GetEngineInstance();
        auto* pEntitySystem = pEngine->GetEntitySystem();
        if (pEntitySystem == nullptr)
            return;
        if (!pEngine->IsUseECSBackend())
        {
            ImGui::TextDisabled("Model reload requires the ECS scene backend");
            return;
        }

        const entt::entity picked = m_SelectedEntity;
        if (picked == entt::null || !pEntitySystem->GetRegistry().valid(picked))
        {
            ImGui::TextDisabled("No object selected. Click a mesh in the viewport.");
            return;
        }
        const auto& reg = pEntitySystem->GetRegistry();
        if (reg.try_get<Scene::ECS::FMeshRendererComponent>(picked) == nullptr)
        {
            ImGui::TextDisabled("Selected entity is not a mesh; ForceReload unavailable.");
            return;
        }

        const entt::entity root = Assets::FModelLoaderECS::FindModelInstanceRoot(reg, picked);
        if (root == entt::null)
        {
            ImGui::TextDisabled("Selected mesh has no model instance (orphan node).");
            return;
        }
        const auto& comp = reg.get<Scene::ECS::FModelInstanceComponent>(root);
        ImGui::TextWrapped("Model: %s", comp.ModelPath.c_str());
        ImGui::Spacing();

        const bool bBusy = pEngine->GetAssetManager() && pEngine->GetAssetManager()->HasPendingRequests();
        ImGui::BeginDisabled(bBusy);
        if (ImGui::Button("ForceReload"))
        {
            Assets::FModelLoaderECS::ReloadModelAsync(comp.ModelPath);
        }
        ImGui::EndDisabled();
        if (bBusy)
            ImGui::TextDisabled("Assets still loading; try again when idle.");
        ImGui::TextDisabled("Force re-cooks and reloads all instances of this model.");
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