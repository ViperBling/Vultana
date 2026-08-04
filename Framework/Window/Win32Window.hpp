#pragma once

#include "Utilities/Math.hpp"

#include <Windows.h>
#include <iostream>
#include <EASTL/string.h>

namespace Window
{
    struct FWin32WindowDesc
    {
        float2 Position = { 0.0f, 0.0f };
        float2 Size = { 1280.0f, 720.0f };
        HINSTANCE Instance = nullptr;
        int ShowCmd = SW_SHOW;
        eastl::string Title = "VultanaEngine";
    };

    class FWin32Window
    {
    public:
        FWin32Window(const FWin32WindowDesc& desc);
        ~FWin32Window();

        void Create();

        void* GetHandle() const { return m_Hwnd; }

    private:
        FWin32WindowDesc m_Desc;
        HWND m_Hwnd = nullptr;
    };
}