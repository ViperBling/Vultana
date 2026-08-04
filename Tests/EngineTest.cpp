#include <gtest/gtest.h>

#include <iostream>

#include "Core/VultanaEngine.hpp"
#include "Window/GLFWindow.hpp"
#include "Scene/ModelLoader.hpp"
#include "Scene/World.hpp"


TEST(EngineTest, Init)
{
    Window::FWindowCreateInfo wndCI {};
    wndCI.Position = { 100, 100 };
    wndCI.Size = { 1280, 720 };

    Window::FGLFWindow window(wndCI);

    Core::FVultanaEngine::GetEngineInstance()->Init(&window, wndCI.Size.x, wndCI.Size.y);

    while (!window.ShouldClose())
    {
        window.PollEvents();

       Core::FVultanaEngine::GetEngineInstance()->Tick();
    }

    Core::FVultanaEngine::GetEngineInstance()->Shutdown();
}