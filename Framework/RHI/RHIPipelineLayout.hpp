#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    class RHIBindGroupLayout;

    struct PipelineConstantLayout
    {
        RHIShaderStageFlags ShaderStage;
        uint32_t Offset;
        uint32_t Size;

        PipelineConstantLayout()
            : ShaderStage(RHIShaderStageFlags::null)
            , Offset(0)
            , Size(0)
        {}
        PipelineConstantLayout(RHIShaderStageFlags inShaderStage, uint32_t inOffset, uint32_t inSize)
            : ShaderStage(inShaderStage)
            , Offset(inOffset)
            , Size(inSize)
        {}
        PipelineConstantLayout& SetShaderStage(RHIShaderStageFlags inShaderStage)
        {
            ShaderStage = inShaderStage;
            return *this;
        }
        PipelineConstantLayout& SetOffset(uint32_t inOffset)
        {
            Offset = inOffset;
            return *this;
        }
        PipelineConstantLayout& SetSize(uint32_t inSize)
        {
            Size = inSize;
            return *this;
        }
    };

    struct PipelineLayoutCreateInfo
    {
        std::vector<const RHIBindGroupLayout*> BindGroupLayouts;
        std::vector<PipelineConstantLayout> PipelineConstantLayouts;
        std::string Name;

        PipelineLayoutCreateInfo() = default;
        PipelineLayoutCreateInfo& AddBindGroupLayout(const RHIBindGroupLayout* bindGroupLayout)
        {
            BindGroupLayouts.emplace_back(bindGroupLayout);
            return *this;
        }
        PipelineLayoutCreateInfo& AddPipelineConstantLayout(const PipelineConstantLayout& pipelineConstantLayout)
        {
            PipelineConstantLayouts.emplace_back(pipelineConstantLayout);
            return *this;
        }
    };

    class RHIPipelineLayout
    {
    public:
        NOCOPY(RHIPipelineLayout)
        virtual ~RHIPipelineLayout() = default;

    protected:
        explicit RHIPipelineLayout(const PipelineLayoutCreateInfo& createInfo) {}
    };
}