#pragma once

#include "RenderGraph.hpp"
#include "RHI/RHI.hpp"

namespace RG
{
    enum class RGBuilderFlag
    {
        None,
        ShaderStagePS,
        ShaderStageNonPS,
    };

    class RGBuilder
    {
    public:
        RGBuilder(RenderGraph* pGraph, RenderGraphPassBase* pPass)
        {
            m_pGraph = pGraph;
            m_pPass = pPass;
        }

        void SkipCulling() { m_pPass->MakeTarget(); }

        template<typename Resource>
        RGHandle Create(const typename Resource::Desc& desc, const eastl::string& name)
        {
            return m_pGraph->Create<Resource>(desc, name);
        }

        RGHandle Import(RHI::RHITexture* texture, RHI::ERHIAccessFlags state)
        {
            return m_pGraph->Import(texture, state);
        }

        RGHandle Read(const RGHandle& input, RHI::ERHIAccessFlags usage, uint32_t subresource = 0)
        {
            assert(usage& (RHI::RHIAccessMaskSRV | RHI::RHIAccessIndirectArgs | RHI::RHIAccessCopySrc));
            assert(RHI::RHI_ALL_SUB_RESOURCE != subresource);

            return m_pGraph->Read(m_pPass, input, usage, subresource);
        }

        RGHandle Read(const RGHandle& input, uint32_t subresource = 0, RGBuilderFlag flag = RGBuilderFlag::None)
        {
            RHI::ERHIAccessFlags state;

            switch (m_pPass->GetType())
            {
            case RenderPassType::Graphics:
                if (flag == RGBuilderFlag::ShaderStagePS) state = RHI::RHIAccessPixelShaderSRV;
                else if (flag == RGBuilderFlag::ShaderStageNonPS) state = RHI::RHIAccessVertexShaderSRV;
                else state = RHI::RHIAccessPixelShaderSRV | RHI::RHIAccessVertexShaderSRV;
                break;
            case RenderPassType::Compute:
            case RenderPassType::AsyncCompute:
                state = RHI::RHIAccessComputeSRV;
                break;
            case RenderPassType::Copy:
                state = RHI::RHIAccessCopySrc;
                break;
            default:
                assert(false);
                break;
            }
            return Read(input, state, subresource);
        }

        RGHandle ReadIndirectArg(const RGHandle& input, uint32_t subresource = 0)
        {
            return Read(input, RHI::RHIAccessIndirectArgs, subresource);
        }

        RGHandle Write(const RGHandle& input, RHI::ERHIAccessFlags usage, uint32_t subreource)
        {
            assert(usage & (RHI::RHIAccessMaskUAV | RHI::RHIAccessCopyDst));
            assert(RHI::RHI_ALL_SUB_RESOURCE != subreource);
            return m_pGraph->Write(m_pPass, input, usage, subreource);
        }

        RGHandle Write(const RGHandle& input, uint32_t subresource = 0, RGBuilderFlag flag = RGBuilderFlag::None)
        {
            RHI::ERHIAccessFlags state;

            switch (m_pPass->GetType())
            {
            case RenderPassType::Graphics:
                if (flag == RGBuilderFlag::ShaderStagePS) state = RHI::RHIAccessPixelShaderUAV;
                else if (flag == RGBuilderFlag::ShaderStageNonPS) state = RHI::RHIAccessVertexShaderUAV;
                else state = RHI::RHIAccessPixelShaderUAV | RHI::RHIAccessVertexShaderUAV;
                break;
            case RenderPassType::Compute:
            case RenderPassType::AsyncCompute:
                state = RHI::RHIAccessComputeUAV | RHI::RHIAccessClearUAV;
                break;
            case RenderPassType::Copy:
                state = RHI::RHIAccessCopyDst;
                break;
            default:
                assert(false);
                break;
            }
            return Write(input, state, subresource);
        }

        RGHandle WriteColor(uint32_t colorIndex, const RGHandle& input, uint32_t subresource, RHI::ERHIRenderPassLoadOp loadOp, float4 clearColor = float4(0.0f, 0.0f, 0.0f, 0.0f))
        {
            assert(m_pPass->GetType() == RenderPassType::Graphics);
            return m_pGraph->WriteColor(m_pPass, colorIndex, input, subresource, loadOp, clearColor);
        }

        RGHandle WriteDepth(const RGHandle& input, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, float clearDepth = 0.0f)
        {
            assert(m_pPass->GetType() == RenderPassType::Graphics);
            return m_pGraph->WriteDepth(m_pPass, input, subresource, depthLoadOp, RHI::ERHIRenderPassLoadOp::DontCare, clearDepth, 0);
        }

        RGHandle WriteDepth(const RGHandle& input, uint32_t subresource, RHI::ERHIRenderPassLoadOp depthLoadOp, RHI::ERHIRenderPassLoadOp stencilLoadOp, float clearDepth = 0.0f, uint32_t clearStencil = 0)
        {
            assert(m_pPass->GetType() == RenderPassType::Graphics);
            return m_pGraph->WriteDepth(m_pPass, input, subresource, depthLoadOp, stencilLoadOp, clearDepth, clearStencil);
        }

        RGHandle ReadDepth(const RGHandle& input, uint32_t subresource = 0)
        {
            assert(m_pPass->GetType() == RenderPassType::Graphics);
            return m_pGraph->ReadDepth(m_pPass, input, subresource);
        }

    private:
        RGBuilder(RGBuilder const&) = delete;
        RGBuilder& operator=(RGBuilder&) = delete;

    private:
        RenderGraph* m_pGraph = nullptr;
        RenderGraphPassBase* m_pPass = nullptr;
    };
}