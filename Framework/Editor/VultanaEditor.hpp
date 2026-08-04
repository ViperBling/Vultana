#pragma once

#include "Renderer/RendererBase.hpp"
#include "Editor/Commands/CommandHistory.hpp"
#include "Editor/Commands/TransformCommand.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/functional.h>

namespace Scene 
{
    class IVisibleObject;
}

namespace Editor
{
    class FVultanaEditor
    {
    public:
        FVultanaEditor(Renderer::FRendererBase* pRenderer);
        ~FVultanaEditor();

        void NewFrame();
        void Tick();
        void Render(RHI::FRHICommandList* pCmdList);

        void AddGUICommand(const eastl::string& window, const eastl::string& section, const eastl::function<void()>& command);
    
    private:
        void BuildDockLayout();
        void DrawToolBar();
        void DrawMenu();
        void DrawGizmo();

        void DrawFrameStats();
        void ShowRenderGraph();
        void FlushPendingTextureDeletions();

        void DrawWindow(const eastl::string& window, bool* pOpen);

    private:
        Renderer::FRendererBase* m_pRenderer = nullptr;
        eastl::unique_ptr<class FImGuiImplement> m_pGUI;

        bool m_bShowImGuiDemo = false;
        bool m_bResetLayout = false;
        bool m_bVSync = false;
        
        bool m_bShowInspector = false;
        bool m_bShowSettings = false;
        bool m_bShowRenderer = false;
        bool m_bShowWorldOutliner = false;
        bool m_bShowGPUDrivenStats = false;
        bool m_bShowMeshlets = false;

        unsigned int m_DockSpace = 0;

        struct FCommand
        {
            eastl::string Section;
            eastl::function<void()> Function;
        };
        using WindowCmd = eastl::vector<FCommand>;
        eastl::hash_map<eastl::string, WindowCmd> m_Commands;

        eastl::hash_map<RHI::FRHIDescriptor*, RenderResources::FTexture2D*> m_FileDialogIcons;
        eastl::vector<RHI::FRHIDescriptor*> m_PendingDeletions;

        enum class ESelectEditMode
        {
            Translate,
            Rotate,
            Scale,
        };
        ESelectEditMode m_SelectEditMode = ESelectEditMode::Translate;
        eastl::unique_ptr<RenderResources::FTexture2D> m_pTranslateIcon;
        eastl::unique_ptr<RenderResources::FTexture2D> m_pRotateIcon;
        eastl::unique_ptr<RenderResources::FTexture2D> m_pScaleIcon;

        // ---- Undo/Redo（Editor::Commands）----
        Editor::Commands::FCommandHistory m_EditHistory;
        bool m_bGizmoDragging = false;
        Scene::IVisibleObject* m_GizmoDragTarget = nullptr;
        Editor::Commands::FTransformSnapshot m_GizmoDragStart;
    };
}