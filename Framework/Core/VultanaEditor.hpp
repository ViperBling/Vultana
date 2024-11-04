#pragma once

#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/functional.h>

namespace Core
{
    class VultanaEditor
    {
    public:
        VultanaEditor();
        ~VultanaEditor();

        void Tick();
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
        bool mbShowImGuiDemo = false;
        bool mbResetLayout = false;

        bool mbShowInspector = false;
        bool mbShowSettings = false;
        bool mbShowRenderer = false;
        bool mbShowWorldOutliner = false;

        unsigned int mDockSpace = 0;

        struct Command
        {
            eastl::string Section;
            eastl::function<void()> Function;
        };
        using WindowCmd = eastl::vector<Command>;
        eastl::hash_map<eastl::string, WindowCmd> mCommands;

        eastl::hash_map<RHI::RHIDescriptor*, RenderResources::Texture2D*> mFileDialogIcons;
        eastl::vector<RHI::RHIDescriptor*> mPendingDeletions;

        enum class ESelectEditMode
        {
            Translate,
            Rotate,
            Scale,
        };
        ESelectEditMode mSelectEditMode = ESelectEditMode::Translate;
        eastl::unique_ptr<RenderResources::Texture2D> mpTranslateIcon;
        eastl::unique_ptr<RenderResources::Texture2D> mpRotateIcon;
        eastl::unique_ptr<RenderResources::Texture2D> mpScaleIcon;
    };
}