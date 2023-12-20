#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    struct HLSLBinding
    {
        RHIHLSLBindingRangeType RangeType;
        uint8_t Index;
    };

    struct GLSLBinding
    {
        uint8_t Index;
    };

    struct ResourceBinding
    {
        RHIBindingType Type;
        union 
        {
            HLSLBinding HLSL;
            GLSLBinding GLSL;
        } Platform;
    };

    struct BindGroupLayoutEntry
    {
        ResourceBinding Binding;
        RHIShaderStageFlags ShaderStage;
    };

    struct BindGroupLayoutCreateInfo
    {
        uint8_t LayoutIndex;
        uint32_t EntryCount;
        const BindGroupLayoutEntry* Entries;
        std::string Name;
    };

    class RHIBindGroupLayout
    {
    public:
        NOCOPY(RHIBindGroupLayout)
        virtual ~RHIBindGroupLayout() = default;

        virtual void Destroy() = 0;

    protected:
        explicit RHIBindGroupLayout(const BindGroupLayoutCreateInfo& createInfo) {}
    };
}