#include "Renderer.hpp"
#include "Core/VultanaEngine.hpp"

#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

namespace Renderer
{
    RendererBase::RendererBase()
    {
        Core::VultanaEngine::GetEngineInstance()->GetWindowHandle()->OnResize(RendererBase::OnWindowResize);
    }

    RendererBase::~RendererBase()
    {
    }

    bool RendererBase::CreateDevice(RHI::ERHIRenderBackend backend, void *windowHandle, uint32_t width, uint32_t height)
    {
        return false;
    }

    void RendererBase::RenderFrame()
    {
    }

    void RendererBase::WaitGPU()
    {
    }

    void RendererBase::OnWindowResize(Window::GLFWindow* wndHandle, uint32_t width, uint32_t height)
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
