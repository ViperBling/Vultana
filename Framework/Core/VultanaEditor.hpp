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
        void DrawMenu();

        void DrawFrameStats();
        void FlushPendingTextureDeletions();

        void DrawWindow(const std::string& window, bool* pOpen);

    private:
        bool mbShowImGuiDemo = false;

        bool mbShowInspector = false;
        bool mbShowSettings = false;

        struct Command
        {
            std::string Section;
            std::function<void()> Function;
        };
        using WindowCmd = std::vector<Command>;
        std::unordered_map<std::string, WindowCmd> mCommands;

        std::vector<RHI::RHIDescriptor*> mPendingDeletions;
    };
}