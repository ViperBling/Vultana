#pragma once

#include "RHI/RHIPCH.hpp"
#include "Utilities/Utility.hpp"
#include "Renderer.hpp"

#include <iostream>
#include <deque>
#include <memory>
#include <functional>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Window
{
    class GLFWindow;
}

namespace Renderer
{
    class RGRenderer : public RendererBase
    {
    public:
        NOCOPY(RGRenderer);
        RGRenderer(Window::GLFWindow* window) : RendererBase(window) {}
        ~RGRenderer();

        void Init(RendererCreateInfo& createInfo) override;
        void Cleanup() override;
        void RenderFrame() override;
    };
}