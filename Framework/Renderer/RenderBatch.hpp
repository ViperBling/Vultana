#pragma once

#include "RHI/RHI.hpp"
#include "Scene/Camera.hpp"
#include <functional>

namespace Renderer
{
    using RenderBatch = std::function<void(RHI::RHICommandList*, const Scene::Camera*)>;
}