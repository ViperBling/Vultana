#include "HiZBuffer.hpp"
#include "Renderer/RendererBase.hpp"

namespace RenderPasses
{
    HiZBuffer::HiZBuffer(Renderer::RendererBase *pRenderer) : mpRenderer(pRenderer)
    {
        // RHI::RHIComputePipelineStateDesc computeDesc;

        // computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "DepthReprojectionCS", RHI::ERHIShaderType::CS);
        // mpDepthReprojectionPSO = pRenderer->GetPipelineState(computeDesc, "HZB Depth Reprojection PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "DepthDilationCS", RHI::ERHIShaderType::CS);
        // mpDepthDilationPSO = pRenderer->GetPipelineState(computeDesc, "HZB Depth Dilation PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "InitHZBCS", RHI::ERHIShaderType::CS);
        // mpInitHZBPSO = pRenderer->GetPipelineState(computeDesc, "HZB Init PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "InitSceneHZBCS", RHI::ERHIShaderType::CS);
        // mpInitSceneHZBPSO = pRenderer->GetPipelineState(computeDesc, "Scene HZB Init PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBuffer.hlsl", "BuildHZBCS", RHI::ERHIShaderType::CS);
        // mpDepthMipFilterPSO = pRenderer->GetPipelineState(computeDesc, "HZB Generate Mips PSO");

        // computeDesc.CS = pRenderer->GetShader("HiZBuffer.hlsl", "BuildHZBCS", RHI::ERHIShaderType::CS, { "MIN_MAX_FILTER=1" });
        // mpDepthMipFilterMinMaxPSO = pRenderer->GetPipelineState(computeDesc, "HZB Generate Mips PSO");
    }

    void HiZBuffer::GenerateCullingHZB1stPhase(RG::RenderGraph *rg)
    {
        // RENDER_GRAPH_EVENT(rg, "HiZBuffer::GenerateCullingHZB1stPhase");
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