#include "Renderer.hpp"

#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace Renderer
{
    struct Vertex
    {
        Math::Vector3 Position;
        Math::Vector3 Color;
    };

    RendererBase::~RendererBase()
    {
        Cleanup();
    }

    void RendererBase::Init(RendererCreateInfo &createInfo)
    {
        InitContext(createInfo);
        InitSwapchain(createInfo);
    
        InitPipelines();
        CreateVertexBuffer();
        InitSyncStructures();
        InitCommands();

        GDebugInfoCallback("RendererBase::Init: Renderer initialized", "Renderer");
    }

    void RendererBase::Cleanup()
    {
        
    }

    void RendererBase::RenderFrame()
    {
        
    }

    void RendererBase::InitContext(RendererCreateInfo &createInfo)
    {
        mSwapchainExtent = { createInfo.Width, createInfo.Height };

        RHI::RHIDeviceDesc deviceDesc {};
        mDevice.reset(CreateRHIDevice(deviceDesc));
        GDebugInfoCallback("Renderer", "RendererBase::InitContext: Device created");
    }

    void RendererBase::InitSwapchain(RendererCreateInfo &createInfo)
    {
        
    }

    void RendererBase::CreateSwapchainImageView()
    {
        
    }

    void RendererBase::InitPipelines()
    {
        
    }

    void RendererBase::CreateVertexBuffer()
    {
        
    }

    void RendererBase::InitSyncStructures()
    {
        
    }

    void RendererBase::InitCommands()
    {
        
    }

    void RendererBase::RecordCommandBuffer()
    {
        
    }

    void RendererBase::SubmitCommandBuffer()
    {
        
    }
} // namespace Vultana::Renderer
