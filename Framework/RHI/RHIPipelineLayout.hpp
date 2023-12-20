#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    class BindGroupLayout;

    struct PipelineConstantLayout
    {
        RHIShaderStageFlags ShaderStage;
        uint32_t Offset;
        uint32_t Size;
    };

    struct PipelineLayoutCretateInfo
    {
        uint32_t BindGroupLayoutCount;
        const BindGroupLayout* const* BindGroupLayouts;
        uint32_t PipelineConstantLayoutCount;
        const PipelineConstantLayout* PipelineConstantLayouts;
        std::string Name;
    };

    class RHIPipelineLayout
    {
    public:
        NOCOPY(RHIPipelineLayout)
        virtual ~RHIPipelineLayout() = default;

        virtual void Destroy() = 0;

    protected:
        explicit RHIPipelineLayout(const PipelineLayoutCretateInfo& createInfo) {}
    };
}