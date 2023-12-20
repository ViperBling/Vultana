#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    struct ShaderModuleCreateInfo
    {
        const void* Code;
        size_t CodeSize;
    };

    class RHIShaderModule
    {
    public:
        NOCOPY(RHIShaderModule)
        virtual ~RHIShaderModule() = default;

        virtual void Destroy() = 0;

    protected:
        explicit RHIShaderModule(const ShaderModuleCreateInfo& createInfo) {}
    };
}