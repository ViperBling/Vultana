#pragma once

#include "RHICommon.hpp"

#include <variant>

namespace RHI
{
    struct HLSLBinding
    {
        RHIHLSLBindingRangeType RangeType;
        uint8_t Index;

        HLSLBinding(RHIHLSLBindingRangeType inRangeType, uint8_t inIndex)
            : RangeType(inRangeType)
            , Index(inIndex) {}
    };

    struct GLSLBinding
    {
        uint8_t Index;

        explicit GLSLBinding(uint8_t inIndex)
            : Index(inIndex) {}
    };

    struct ResourceBinding
    {
        RHIBindingType Type;
        std::variant<HLSLBinding, GLSLBinding> PlatformBinding;

        ResourceBinding(RHIBindingType inType, const std::variant<HLSLBinding, GLSLBinding>& inPlatformBinding)
            : Type(inType)
            , PlatformBinding(inPlatformBinding) {}
    };

    struct BindGroupLayoutEntry
    {
        ResourceBinding Binding;
        RHIShaderStageFlags ShaderStage;

        BindGroupLayoutEntry(const ResourceBinding& inBinding, RHIShaderStageFlags inShaderStage)
            : Binding(inBinding)
            , ShaderStage(inShaderStage) {}
    };

    struct BindGroupLayoutCreateInfo
    {
        uint8_t LayoutIndex;
        std::vector<BindGroupLayoutEntry> Entries;
        std::string Name;

        explicit BindGroupLayoutCreateInfo(uint8_t inLayoutIndex, std::string inName = "")
            : LayoutIndex(inLayoutIndex)
            , Name(std::move(inName)) 
        {}
        BindGroupLayoutCreateInfo& AddEntry(const BindGroupLayoutEntry& entry)
        {
            Entries.emplace_back(entry);
            return *this;
        }
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