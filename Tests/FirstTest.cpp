#include <iostream>

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

    while (!window.ShouldClose())
    {
        window.PollEvents();
    }

    return 0;
}