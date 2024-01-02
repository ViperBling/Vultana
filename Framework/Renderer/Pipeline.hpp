#pragma once

#include <unordered_map>

#include "RHI/RHIPCH.hpp"
#include "Shader.hpp"

namespace Renderer
{
    class PipelineLayout;
    class ComputePipelineState;

    using BindingMap = std::unordered_map<std::string, std::pair<RHI::RHIShaderStageFlags, RHI::ResourceBinding>>;
}