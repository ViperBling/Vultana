#include <iostream>

#include "Core/VultanaEngine.hpp"
#include "Windows/GLFWindow.hpp"
#include "Scene/ModelLoader.hpp"
#include "Scene/World.hpp"

#include "RHI/RHIInstance.hpp"
#include "RHI/RHIGPU.hpp"

using namespace Vultana;

int main()
{
    WindowCreateInfo wndCI {};
    wndCI.Position = { 100, 100 };
    wndCI.Size = { 1280, 720 };

    // GLFWindow window(wndCI);
    // World world;

    // Engine::GetEngineInstance()->Init(&window, wndCI.Size.x, wndCI.Size.y);

    // while (!window.ShouldClose())
    // {
    //     window.PollEvents();

    //    Engine::GetEngineInstance()->Tick();
    // }

    // Engine::GetEngineInstance()->Shutdown();

    auto instance = RHIInstance::GetInstanceByRHIBackend(RHIRenderBackend::Vulkan);
    auto GPU = instance->GetGPU(0);

    std::vector<QueueInfo> queueCI = { {RHICommandQueueType::Graphics, 1} };
    DeviceCreateInfo deviceCI {};
    deviceCI.QueueCreateInfos = queueCI.data();
    deviceCI.QueueCreateInfoCount = queueCI.size();
    auto Device = GPU->RequestDevice(deviceCI);
    auto GraphicsQueue = Device->GetQueue(RHICommandQueueType::Graphics, 0);

    return 0;
}