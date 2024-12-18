#include "Win32Window.hpp"
#include "Core/VultanaEngine.hpp"
#include "Utilities/String.hpp"

#include <ImGui/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    switch (message)
    {
    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal(hWnd, LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    }
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

namespace Window
{
    Win32Window::Win32Window(const Win32WindowDesc &desc)
    {
        mDesc = desc;
    }

    Win32Window::~Win32Window()
    {

    }

    void Win32Window::Create()
    {
        WNDCLASSEX wndClass = {0};
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = WindowProc;
        wndClass.hInstance = mDesc.Instance;
        wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndClass.lpszClassName = L"VultanaEngine";
        RegisterClassEx(&wndClass);

        RECT windowRect = { 0, 0, (LONG)mDesc.Size.x, (LONG)mDesc.Size.y };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        mHwnd = CreateWindow(
            wndClass.lpszClassName, 
            L"VultanaEngine",
            WS_OVERLAPPEDWINDOW, 
            mDesc.Position.x, mDesc.Position.y, 
            windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, 
            nullptr, nullptr, mDesc.Instance, nullptr);

        ShowWindow(mHwnd, mDesc.ShowCmd);
    }
}