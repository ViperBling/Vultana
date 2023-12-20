#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    class RHIInstance;

    struct RHIModuleCreateInfo
    {
        RHIRenderBackend RenderBackend = RHIRenderBackend::Vulkan;
    };

    class RHIModule
    {
    public:
        RHIModule() = default;
        virtual ~RHIModule() = default;
        virtual RHIInstance* GetRHIInstance() = 0;

        explicit RHIModule(const RHIRenderBackend& backend) {}
    };
}