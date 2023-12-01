#include <iostream>

#include "Core/VultanaEngine.hpp"
#include "Windows/GLFWindow.hpp"
#include "Scene/ModelLoader.hpp"
#include "Scene/World.hpp"

int main()
{
    Vultana::WindowCreateInfo wndCI {};
    wndCI.Position = { 100, 100 };
    wndCI.Size = { 1280, 720 };

    Vultana::Window window(wndCI);
    Vultana::Scene::World world;

    Vultana::Engine::GetEngineInstance()->Init(window.GetNativeHandle(), wndCI.Size.x, wndCI.Size.y);

    while (!window.ShouldClose())
    {
        window.PollEvents();

        Vultana::Engine::GetEngineInstance()->Tick();
        // std::cout << "Frame time: " << Vultana::Engine::GetEngineInstance()->GetDeltaTime() * 1000.0 << std::endl;
    }

    Vultana::Engine::GetEngineInstance()->Shutdown();

    return 0;
}