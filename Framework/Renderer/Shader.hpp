#pragma once

#include <vector>
#include <unordered_map>
#include <utility>
#include <tuple>
#include <functional>

#include "RHI/RHICommon.hpp"
#include "RHI/RHIShaderModule.hpp"
#include "RHI/RHIBindGroupLayout.hpp"

namespace Vultana
{
    class Shader {};

    using ShaderTypeKey = uint64_t;
    using VariantKey = uint64_t;
    using ShaderByteCode = std::vector<uint8_t>;
    using ShaderStage = RHIShaderStageBits;

    struct ShaderReflectionData
    {
        using LayoutIndex = uint8_t;
        std::unordered_map<std::string, std::pair<LayoutIndex, ResourceBinding>> ResourceBindings;
    };
}