#pragma once

#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/RenderResources/TypedBuffer.hpp"

namespace Renderer
{
    class FHiZBuffer
    {
    public:
        FHiZBuffer(Renderer::FRendererBase* pRenderer);

        void GenerateCullingHZB1stPhase(RG::FRenderGraph* rg);
        void GenerateCullingHZB2ndPhase(RG::FRenderGraph* rg, RG::FRGHandle depthRT);
        void GenerateSceneHZB(RG::FRenderGraph* rg, RG::FRGHandle depthRT);

        RG::FRGHandle GetCullingHZBMip1stPhase(uint32_t mip) const;
        RG::FRGHandle GetCullingHZBMip2ndPhase(uint32_t mip) const;
        RG::FRGHandle GetSceneHZBMip(uint32_t mip) const;

        uint32_t GetHZBMipCount() const { return m_HZBMipCount; }
        uint32_t GetHZBWidth() const { return m_HZBSize.x; }
        uint32_t GetHZBHeight() const { return m_HZBSize.y; }

    private:
        void CalcHZBSize();

        void ReprojectDepth(RHI::FRHICommandList* pCmdList, RG::FRGTexture* reprojectedDepthTexture);
        void DilationDepth(RHI::FRHICommandList* pCmdList, RG::FRGTexture* reprojectedDepthSRV, RG::FRGTexture* hzbMip0UAV);
        void BuildHZB(RHI::FRHICommandList* pCmdList, RG::FRGTexture* texture, bool minMax = false);
        void InitHZB(RHI::FRHICommandList* pCmdList, RG::FRGTexture* inputDepthSRV, RG::FRGTexture* hzbMip0UAV, bool minMax = false);

    private:
        Renderer::FRendererBase* m_pRenderer = nullptr;

        RHI::FRHIPipelineState* m_pDepthReprojectionPSO = nullptr;
        RHI::FRHIPipelineState* m_pDepthDilationPSO = nullptr;
        RHI::FRHIPipelineState* m_pDepthMipFilterPSO = nullptr;
        RHI::FRHIPipelineState* m_pInitHZBPSO = nullptr;

        RHI::FRHIPipelineState* m_pInitSceneHZBPSO = nullptr;
        RHI::FRHIPipelineState* m_pDepthMipFilterMinMaxPSO = nullptr;

        uint32_t m_HZBMipCount = 0;
        uint2 m_HZBSize;

        static const uint32_t MAX_HZB_MIP_COUNT = 13;
        RG::FRGHandle m_CullingHZBMips1stPhase[MAX_HZB_MIP_COUNT] = {};
        RG::FRGHandle m_CullingHZBMips2ndPhase[MAX_HZB_MIP_COUNT] = {};
        RG::FRGHandle m_SceneHZBMips[MAX_HZB_MIP_COUNT] = {};
    };
}