#include <iostream>

#include "Utilities/GLFWindow.hpp"
#include "Utilities/AssetsManager.hpp"

int main()
{
    Vultana::WindowCreateInfo wndCI {};
    wndCI.Position = { 100, 100 };
    wndCI.Size = { 1280, 720 };

    Vultana::Window window(wndCI);

    while (!window.ShouldClose())
    {
        window.PollEvents();
    }

    return 0;
}