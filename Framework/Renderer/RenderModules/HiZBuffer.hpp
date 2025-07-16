#pragma once

#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/RenderResources/TypedBuffer.hpp"

namespace Renderer
{
    class HiZBuffer
    {
    public:
        HiZBuffer(Renderer::RendererBase* pRenderer);

        void GenerateCullingHZB1stPhase(RG::RenderGraph* rg);
        void GenerateCullingHZB2ndPhase(RG::RenderGraph* rg, RG::RGHandle depthRT);
        void GenerateSceneHZB(RG::RenderGraph* rg, RG::RGHandle depthRT);

        RG::RGHandle GetCullingHZBMip1stPhase(uint32_t mip) const;
        RG::RGHandle GetCullingHZBMip2ndPhase(uint32_t mip) const;
        RG::RGHandle GetSceneHZBMip(uint32_t mip) const;

        uint32_t GetHZBMipCount() const { return m_HZBMipCount; }
        uint32_t GetHZBWidth() const { return m_HZBSize.x; }
        uint32_t GetHZBHeight() const { return m_HZBSize.y; }

    private:
        void CalcHZBSize();

        void ReprojectDepth(RHI::RHICommandList* pCmdList, RG::RGTexture* reprojectionDepthTexture);
        void DilationDepth(RHI::RHICommandList* pCmdList, RG::RGTexture* reprojectionDepthSRV, RG::RGTexture* hzbMip0UAV);
        void BuildHZB(RHI::RHICommandList* pCmdList, RG::RGTexture* texture, bool minMax = false);
        void InitHZB(RHI::RHICommandList* pCmdList, RG::RGTexture* inputDepthSRV, RG::RGTexture* hzbMip0UAV, bool minMax = false);

    private:
        Renderer::RendererBase* m_pRenderer = nullptr;

        RHI::RHIPipelineState* m_pDepthReprojectionPSO = nullptr;
        RHI::RHIPipelineState* m_pDepthDilationPSO = nullptr;
        RHI::RHIPipelineState* m_pDepthMipFilterPSO = nullptr;
        RHI::RHIPipelineState* m_pInitHZBPSO = nullptr;

        RHI::RHIPipelineState* m_pInitSceneHZBPSO = nullptr;
        RHI::RHIPipelineState* m_pDepthMipFilterMinMaxPSO = nullptr;

        uint32_t m_HZBMipCount = 0;
        uint2 m_HZBSize;

        static const uint32_t MAX_HZB_MIP_COUNT = 13;
        RG::RGHandle m_CullingHZBMips1stPhase[MAX_HZB_MIP_COUNT] = {};
        RG::RGHandle m_CullingHZBMips2ndPhase[MAX_HZB_MIP_COUNT] = {};
        RG::RGHandle m_SceneHZBMips[MAX_HZB_MIP_COUNT] = {};
    };
}