#include <iostream>

#include "Core/VultanaEngine.hpp"
#include "Windows/GLFWindow.hpp"
#include "Scene/ModelLoader.hpp"
#include "Scene/World.hpp"

using namespace Vultana;

int main()
{
    WindowCreateInfo wndCI {};
    wndCI.Position = { 100, 100 };
    wndCI.Size = { 1280, 720 };

    Window window(wndCI);
    Scene::World world;

    Engine::GetEngineInstance()->Init(window.GetNativeHandle(), wndCI.Size.x, wndCI.Size.y);

    while (!window.ShouldClose())
    {
        window.PollEvents();

       Engine::GetEngineInstance()->Tick();
    }

    Engine::GetEngineInstance()->Shutdown();

    return 0;
}