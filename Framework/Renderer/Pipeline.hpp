#pragma once

#include <unordered_map>

#include "RHI/RHIPCH.hpp"
#include "Shader.hpp"

namespace Vultana
{
    class RGPipelineLayout;
    class RGComputePipelineState;

    using BindingMap = std::unordered_map<std::string, std::pair<RHIShaderStageFlags, ResourceBinding>>;
}