#pragma once

#include "Utilities/Math.hpp"

#include <Windows.h>
#include <iostream>
#include <EASTL/string.h>

namespace Window
{
    struct Win32WindowDesc
    {
        float2 Position = { 0.0f, 0.0f };
        float2 Size = { 1280.0f, 720.0f };
        HINSTANCE Instance = nullptr;
        int ShowCmd = SW_SHOW;
        eastl::string Title = "VultanaEngine";
    };

    class Win32Window
    {
    public:
        Win32Window(const Win32WindowDesc& desc);
        ~Win32Window();

        void Create();

        void* GetHandle() const { return mHwnd; }

    private:
        Win32WindowDesc mDesc;
        HWND mHwnd = nullptr;
    };
}