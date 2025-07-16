#include "HiZBuffer.hpp"
#include "Renderer/RendererBase.hpp"

namespace Renderer
{
    HiZBuffer::HiZBuffer(Renderer::RendererBase *pRenderer) : m_pRenderer(pRenderer)
    {
        // RHI::RHIComputePipelineStateDesc computeDesc;

        // computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "DepthReprojectionCS", RHI::ERHIShaderType::CS);
        // m_pDepthReprojectionPSO = pRenderer->GetPipelineState(computeDesc, "HZB Depth Reprojection PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "DepthDilationCS", RHI::ERHIShaderType::CS);
        // m_pDepthDilationPSO = pRenderer->GetPipelineState(computeDesc, "HZB Depth Dilation PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "InitHZBCS", RHI::ERHIShaderType::CS);
        // m_pInitHZBPSO = pRenderer->GetPipelineState(computeDesc, "HZB Init PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "InitSceneHZBCS", RHI::ERHIShaderType::CS);
        // m_pInitSceneHZBPSO = pRenderer->GetPipelineState(computeDesc, "Scene HZB Init PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBuffer.hlsl", "BuildHZBCS", RHI::ERHIShaderType::CS);
        // m_pDepthMipFilterPSO = pRenderer->GetPipelineState(computeDesc, "HZB Generate Mips PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBuffer.hlsl", "BuildHZBCS", RHI::ERHIShaderType::CS, { "MIN_MAX_FILTER=1" });
        // m_pDepthMipFilterMinMaxPSO = pRenderer->GetPipelineState(computeDesc, "HZB Generate Mips PSO");
    }

    void HiZBuffer::GenerateCullingHZB1stPhase(RG::RenderGraph *rg)
    {
        RENDER_GRAPH_EVENT(rg, "HiZBuffer::GenerateCullingHZB1stPhase");

        CalcHZBSize();

        struct FDepthReprojectionData
        {
            RG::RGHandle PrevDepth;
            RG::RGHandle ReprojectedDepth;
        };

        // auto reprojectionPass = rg->AddPass<FDepthReprojectionData>("Depth Reprojection", RG::RenderPassType::Compute,
        // [&](FDepthReprojectionData& data, RG::RGBuilder& builder)
        // {
        //     // data.PrevDepth = builder.Read(m_pRenderer->GetPre)
        // });
    }

    void HiZBuffer::GenerateCullingHZB2ndPhase(RG::RenderGraph *rg, RG::RGHandle depthRT)
    {
        // RENDER_GRAPH_EVENT(rg, "HiZBuffer::GenerateCullingHZB2ndPhase");
    }

    void HiZBuffer::GenerateSceneHZB(RG::RenderGraph *rg, RG::RGHandle depthRT)
    {
    }

    RG::RGHandle HiZBuffer::GetCullingHZBMip1stPhase(uint32_t mip) const
    {
        return RG::RGHandle();
    }

    RG::RGHandle HiZBuffer::GetCullingHZBMip2ndPhase(uint32_t mip) const
    {
        return RG::RGHandle();
    }

    RG::RGHandle HiZBuffer::GetSceneHZBMip(uint32_t mip) const
    {
        return RG::RGHandle();
    }

    void HiZBuffer::CalcHZBSize()
    {
    }

    void HiZBuffer::ReprojectDepth(RHI::RHICommandList *pCmdList, RG::RGTexture *reprojectionDepthTexture)
    {
    }

    void HiZBuffer::DilationDepth(RHI::RHICommandList *pCmdList, RG::RGTexture *reprojectionDepthSRV, RG::RGTexture *hzbMip0UAV)
    {
    }

    void HiZBuffer::BuildHZB(RHI::RHICommandList *pCmdList, RG::RGTexture *texture, bool minMax)
    {
    }

    void HiZBuffer::InitHZB(RHI::RHICommandList *pCmdList, RG::RGTexture *inputDepthSRV, RG::RGTexture *hzbMip0UAV, bool minMax)
    {
    }
}