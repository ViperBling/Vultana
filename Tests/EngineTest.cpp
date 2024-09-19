#include <gtest/gtest.h>

#include <iostream>

#include "Core/VultanaEngine.hpp"
#include "Windows/GLFWindow.hpp"
#include "Scene/ModelLoader.hpp"
#include "Scene/World.hpp"


TEST(EngineTest, Init)
{
    Window::WindowCreateInfo wndCI {};
    wndCI.Position = { 100, 100 };
    wndCI.Size = { 1280, 720 };

    Window::GLFWindow window(wndCI);

    Core::VultanaEngine::GetEngineInstance()->Init(&window, wndCI.Size.x, wndCI.Size.y);

    while (!window.ShouldClose())
    {
        window.PollEvents();

       Core::VultanaEngine::GetEngineInstance()->Tick();
    }

    Core::VultanaEngine::GetEngineInstance()->Shutdown();
}