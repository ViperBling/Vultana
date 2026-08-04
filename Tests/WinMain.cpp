#include "Core/VultanaEngine.hpp"
#include "Window/Win32Window.hpp"

#include <rpmalloc/rpmalloc.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nShowCmd)
{
    rpmalloc_initialize();
    Window::FWin32WindowDesc wndDesc;
    wndDesc.Position = { 100.0f, 100.0f };
    wndDesc.Size = { 1280.0f, 720.0f };
    wndDesc.Instance = hInstance;
    wndDesc.ShowCmd = nShowCmd;
    wndDesc.Title = "VultanaEngine";

    Window::FWin32Window window(wndDesc);
    window.Create();

    Core::FVultanaEngine::GetEngineInstance()->Init(window.GetHandle(), wndDesc.Size.x, wndDesc.Size.y);

    MSG msg = { };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Core::FVultanaEngine::GetEngineInstance()->Tick();
    }
    Core::FVultanaEngine::GetEngineInstance()->Shutdown();

    return 0;
}