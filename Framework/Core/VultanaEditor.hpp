#pragma once

#include "Renderer/RendererBase.hpp"

#include <unordered_map>
#include <functional>

namespace Core
{
    class VultanaEditor
    {
    public:
        VultanaEditor();
        ~VultanaEditor();

        void Tick();
        void AddGUICommand(const std::string& window, const std::string& section, const std::function<void()>& command);
    
    private:
        void BuildDockLayout();
        void DrawToolBar();
        void DrawMenu();
        void DrawGizmo();

        void DrawFrameStats();
        void ShowRenderGraph();
        void FlushPendingTextureDeletions();

        void DrawWindow(const std::string& window, bool* pOpen);

    private:
        bool mbShowImGuiDemo = false;
        bool mbResetLayout = false;

        bool mbShowInspector = false;
        bool mbShowSettings = false;
        bool mbShowRenderer = false;

        unsigned int mDockSpace = 0;

        struct Command
        {
            std::string Section;
            std::function<void()> Function;
        };
        using WindowCmd = std::vector<Command>;
        std::unordered_map<std::string, WindowCmd> mCommands;

        std::unordered_map<RHI::RHIDescriptor*, RenderResources::Texture2D*> mFileDialogIcons;
        std::vector<RHI::RHIDescriptor*> mPendingDeletions;

        enum class ESelectEditMode
        {
            Translate,
            Rotate,
            Scale,
        };
        ESelectEditMode mSelectEditMode = ESelectEditMode::Translate;
        std::unique_ptr<RenderResources::Texture2D> mpTranslateIcon;
        std::unique_ptr<RenderResources::Texture2D> mpRotateIcon;
        std::unique_ptr<RenderResources::Texture2D> mpScaleIcon;
    };
}