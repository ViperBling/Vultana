#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    class RHIBindGroupLayout;
    class RHIPipelineLayout;
    class RHIShaderModule;

    struct VertexAttribute
    {
        RHIVertexFormat Format;
        size_t Offset;
        const char* SemanticName;
        uint8_t SemanticIndex;
    };

    struct VertexBufferLayout
    {
        size_t Stride;
        RHIVertexStepMode StepMode;
        uint32_t AttributeCount;
        const VertexAttribute* Attributes;
    };

    struct VertexState
    {
        uint32_t BufferLayoutCount = 0;
        const VertexBufferLayout* BufferLayouts = nullptr;
    };

    struct PrimitiveState
    {
        RHIPrimitiveTopologyType TopologyType = RHIPrimitiveTopologyType::Triangle;
        RHIIndexFormat IndexFormat = RHIIndexFormat::UINT_16;
        RHIFrontFace FrontFace = RHIFrontFace::CounterClockwise;
        RHICullMode CullMode = RHICullMode::None;
        bool bDepthClip = false;
    };

    struct StencilFaceState
    {
        RHICompareOp CompareOp = RHICompareOp::Always;
        RHIStencilOp FailOp = RHIStencilOp::Keep;
        RHIStencilOp DepthFailOp = RHIStencilOp::Keep;
        RHIStencilOp PassOp = RHIStencilOp::Keep;
    };

    struct DepthStencilState
    {
        bool bDepthEnable = false;
        bool bStencilEnable = false;
        RHIFormat Format = RHIFormat::Count;
        RHICompareOp DepthCompareOp = RHICompareOp::Always;
        StencilFaceState StencilFront;
        StencilFaceState StencilBack;
        uint8_t StencilReadMask = 0xFF;
        uint8_t StencilWriteMask = 0xFF;
        int32_t DepthBias = 0;
        float DepthBiasSlopScale = 0.f;
        float DepthBiasClamp = 0.f;
    };

    struct MultiSampleState
    {
        uint8_t SampleCount = 1;
        uint8_t SampleMask = 0xFFFFFFFF;
        bool bAlphaToCoverage = false;
    };

    struct BlendComponent
    {
        RHIBlendOp Op = RHIBlendOp::Add;
        RHIBlendFactor Src = RHIBlendFactor::One;
        RHIBlendFactor Dst = RHIBlendFactor::Zero;
    };

    struct BlendState
    {
        BlendComponent Color;
        BlendComponent Alpha;
    };

    struct ColorTargetState
    {
        RHIFormat Format = RHIFormat::BGRA8_UNORM;
        BlendState Blend;
        RHIColorWriteFlags WriteFlags = RHIColorWriteBits::Red | RHIColorWriteBits::Green | RHIColorWriteBits::Blue | RHIColorWriteBits::Alpha;
    };

    struct FragmentState
    {
        uint8_t ColorTargetCount = 0;
        const ColorTargetState* ColorTargets = nullptr;
    };

    struct ComputePipelineCreateInfo
    {
        RHIPipelineLayout* PipelineLayout = nullptr;
        RHIShaderModule* Shader = nullptr;
    };

    struct GraphicsPipelineCreateInfo
    {
        RHIPipelineLayout* PipelineLayout = nullptr;

        RHIShaderModule* VertexShader = nullptr;
        RHIShaderModule* FragmentShader = nullptr;
        RHIShaderModule* GeometryShader = nullptr;
        RHIShaderModule* DomainShader = nullptr;
        RHIShaderModule* HullShader = nullptr;

        VertexState VertexState;
        PrimitiveState PrimitiveState;
        DepthStencilState DepthStencilState;
        MultiSampleState MultiSampleState;
        FragmentState FragState;

        std::string Name;
    };

    class RHIPipeline
    {
    public:
        NOCOPY(RHIPipeline)
        virtual ~RHIPipeline() = default;

        virtual void Destroy() = 0;

    protected:
        RHIPipeline() = default;
    };

    class RHIComputePipeline : public RHIPipeline
    {
    public:
        NOCOPY(RHIComputePipeline)
        ~RHIComputePipeline() override;

        void Destroy() override;

    protected:
        explicit RHIComputePipeline(const ComputePipelineCreateInfo& createInfo) {}
    };

    class RHIGraphicsPipeline : public RHIPipeline
    {
    public:
        NOCOPY(RHIGraphicsPipeline)
        ~RHIGraphicsPipeline() override;

        void Destroy() override;

    protected:
        explicit RHIGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) {}
    };
}