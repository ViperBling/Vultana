#include "RendererBase.hpp"
#include "Core/VultanaEngine.hpp"

#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

namespace Renderer
{
    RendererBase::RendererBase()
    {
        Core::VultanaEngine::GetEngineInstance()->GetWindowHandle()->OnResize(&RendererBase::OnWindowResize);
    }

    RendererBase::~RendererBase()
    {
        WaitGPU();
    }

    bool RendererBase::CreateDevice(RHI::ERHIRenderBackend backend, void *windowHandle, uint32_t width, uint32_t height)
    {
        mDisplayWidth = width;
        mDisplayHeight = height;
    }

    void RendererBase::RenderFrame()
    {
    }

    void RendererBase::WaitGPU()
    {
        if (mpFrameFence)
        {
            mpFrameFence->Wait(mCurrentFrameFenceValue);
        }
    }

    void RendererBase::OnWindowResize(Window::GLFWindow& wndHandle, uint32_t width, uint32_t height)
    {
    }

    void RendererBase::BeginFrame()
    {
    }

    void RendererBase::UploadResource()
    {
    }

    void RendererBase::Render()
    {
    }

    void RendererBase::EndFrame()
    {
    }
} // namespace Vultana::Renderer
