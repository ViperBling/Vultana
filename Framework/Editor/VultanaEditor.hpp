#pragma once

#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/functional.h>

namespace Editor
{
    class VultanaEditor
    {
    public:
        VultanaEditor(Renderer::RendererBase* pRenderer);
        ~VultanaEditor();

        void NewFrame();
        void Tick();
        void Render(RHI::RHICommandList* pCmdList);

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
        Renderer::RendererBase* m_pRenderer = nullptr;
        eastl::unique_ptr<class ImGuiImplement> m_pGUI;

        bool m_bShowImGuiDemo = false;
        bool m_bResetLayout = false;
        bool m_bVSync = false;
        
        bool m_bShowInspector = false;
        bool m_bShowSettings = false;
        bool m_bShowRenderer = false;
        bool m_bShowWorldOutliner = false;

        unsigned int m_DockSpace = 0;

        struct Command
        {
            eastl::string Section;
            eastl::function<void()> Function;
        };
        using WindowCmd = eastl::vector<Command>;
        eastl::hash_map<eastl::string, WindowCmd> m_Commands;

        eastl::hash_map<RHI::RHIDescriptor*, RenderResources::Texture2D*> m_FileDialogIcons;
        eastl::vector<RHI::RHIDescriptor*> m_PendingDeletions;

        enum class ESelectEditMode
        {
            Translate,
            Rotate,
            Scale,
        };
        ESelectEditMode m_SelectEditMode = ESelectEditMode::Translate;
        eastl::unique_ptr<RenderResources::Texture2D> m_pTranslateIcon;
        eastl::unique_ptr<RenderResources::Texture2D> m_pRotateIcon;
        eastl::unique_ptr<RenderResources::Texture2D> m_pScaleIcon;
    };
}