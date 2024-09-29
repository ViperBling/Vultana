#include "Core/VultanaEngine.hpp"

#include <ImGui/imgui_impl_win32.h>

#include <Windows.h>

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
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nShowCmd)
{
    WNDCLASSEX wndClass = {0};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WindowProc;
    wndClass.hInstance = hInstance;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpszClassName = "VultanaEngine";
    RegisterClassEx(&wndClass);

    RECT desktopRect;
    GetClientRect(GetDesktopWindow(), &desktopRect);

    const unsigned int desktopWidth = desktopRect.right - desktopRect.left;
    const unsigned int desktopHeight = desktopRect.bottom - desktopRect.top;

    const unsigned int windowWidth = min(desktopWidth * 0.6, 1920);
    const unsigned int windowHeight = min(desktopHeight * 0.6, 1080);

    RECT windowRect = { 0, 0, (LONG)windowWidth, (LONG)windowHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int x = (desktopWidth - (windowRect.right - windowRect.left)) / 2;
    int y = (desktopHeight - (windowRect.bottom - windowRect.top)) / 2;

    HWND hWnd = CreateWindow(
        wndClass.lpszClassName, 
        "VultanaEngine", 
        WS_OVERLAPPEDWINDOW, 
        x, y, 
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, 
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nShowCmd);

    Core::VultanaEngine::GetEngineInstance()->Init(hWnd, windowWidth, windowHeight);

    MSG msg = { };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Core::VultanaEngine::GetEngineInstance()->Tick();
    }
    Core::VultanaEngine::GetEngineInstance()->Shutdown();

    return 0;
}