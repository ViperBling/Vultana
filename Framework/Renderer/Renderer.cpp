#include "Renderer.hpp"

#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace Vultana
{
    RendererBase::~RendererBase()
    {
    }

    void RendererBase::Init(RendererCreateInfo &createInfo)
    {
        InitContext(createInfo);
        InitSwapchain(createInfo);
        // InitCommands();
        // InitPipelines();
        // InitDescriptors();
        // InitSyncStructures();
    }

    void RendererBase::Cleanup()
    {
    }

    void RendererBase::RenderFrame()
    {
    }

    void RendererBase::InitContext(RendererCreateInfo &createInfo)
    {
        mInstance = RHIInstance::GetInstanceByRHIBackend(RHIRenderBackend::Vulkan);
        mGPU = mInstance->GetGPU(0);

        std::vector<QueueInfo> queueInfos = {
            { RHICommandQueueType::Graphics, 1 },
        };
        DeviceCreateInfo deviceCI {};
        deviceCI.QueueCreateInfoCount = queueInfos.size();
        deviceCI.QueueCreateInfos = queueInfos.data();
        mDevice = std::unique_ptr<RHIDevice>(mGPU->RequestDevice(deviceCI));
        mQueue = mDevice->GetQueue(RHICommandQueueType::Graphics, 0);
    }

    void RendererBase::InitSwapchain(RendererCreateInfo &createInfo)
    {
        static std::vector<RHIFormat> swapchainFormats = {
            RHIFormat::RGBA8_UNORM,
            RHIFormat::BGRA8_UNORM,
        };

        SurfaceCreateInfo surfaceCI {};
        surfaceCI.Window = mWndHandle->GetNativeHandle();
        mSurface = std::unique_ptr<RHISurface>(mDevice->CreateSurface(surfaceCI));

        GDebugInfoCallback("RendererBase::InitSwapchain: Surface created", "Renderer");
    }

    void RendererBase::InitCommands()
    {
    }

    void RendererBase::InitPipelines()
    {
    }

    void RendererBase::InitDescriptors()
    {
    }

    void RendererBase::InitSyncStructures()
    {
    }

} // namespace Vultana::Renderer
