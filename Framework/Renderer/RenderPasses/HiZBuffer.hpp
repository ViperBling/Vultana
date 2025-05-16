#pragma once

#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/RenderResources/TypedBuffer.hpp"

namespace Renderer
{
    class RendererBase;
}

namespace RenderPasses
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

        uint32_t GetHZBMipCount() const { return mHZBMipCount; }
        uint32_t GetHZBWidth() const { return mHZBSize.x; }
        uint32_t GetHZBHeight() const { return mHZBSize.y; }

    private:
        void CalcHZBSize();

        void ReprojectDepth(RHI::RHICommandList* pCmdList, RG::RGTexture* reprojectionDepthTexture);
        void DilationDepth(RHI::RHICommandList* pCmdList, RG::RGTexture* reprojectionDepthSRV, RG::RGTexture* hzbMip0UAV);
        void BuildHZB(RHI::RHICommandList* pCmdList, RG::RGTexture* texture, bool minMax = false);
        void InitHZB(RHI::RHICommandList* pCmdList, RG::RGTexture* inputDepthSRV, RG::RGTexture* hzbMip0UAV, bool minMax = false);

    private:
        Renderer::RendererBase* mpRenderer = nullptr;

        RHI::RHIPipelineState* mpDepthReprojectionPSO = nullptr;
        RHI::RHIPipelineState* mpDepthDilationPSO = nullptr;
        RHI::RHIPipelineState* mpDepthMipFilterPSO = nullptr;
        RHI::RHIPipelineState* mpInitHZBPSO = nullptr;

        RHI::RHIPipelineState* mpInitSceneHZBPSO = nullptr;
        RHI::RHIPipelineState* mpDepthMipFilterMinMaxPSO = nullptr;

        uint32_t mHZBMipCount = 0;
        uint2 mHZBSize;

        static const uint32_t MAX_HZB_MIP_COUNT = 13;
        RG::RGHandle mCullingHZBMips1stPhase[MAX_HZB_MIP_COUNT] = {};
        RG::RGHandle mCullingHZBMips2ndPhase[MAX_HZB_MIP_COUNT] = {};
        RG::RGHandle mSceneHZBMips[MAX_HZB_MIP_COUNT] = {};
    };
}