#include "HiZBuffer.hpp"
#include "Renderer/RendererBase.hpp"
#include "Renderer/ClearUAV.hpp"

#define A_CPU
#include "Common/FFX_A.hpp"
#include "Common/FFX_SPD.hpp"

namespace Renderer
{
    HiZBuffer::HiZBuffer(Renderer::RendererBase *pRenderer) : m_pRenderer(pRenderer)
    {
        RHI::RHIComputePipelineStateDesc computeDesc;

        computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "DepthReprojectionCS", RHI::ERHIShaderType::CS);
        m_pDepthReprojectionPSO = pRenderer->GetPipelineState(computeDesc, "HZB Depth Reprojection PSO");

        computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "DepthDilationCS", RHI::ERHIShaderType::CS);
        m_pDepthDilationPSO = pRenderer->GetPipelineState(computeDesc, "HZB Depth Dilation PSO");

        computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "InitHZBCS", RHI::ERHIShaderType::CS);
        m_pInitHZBPSO = pRenderer->GetPipelineState(computeDesc, "HZB Init PSO");

        computeDesc.CS = pRenderer->GetShader("HiZBufferReprojection.hlsl", "InitSceneHZBCS", RHI::ERHIShaderType::CS);
        m_pInitSceneHZBPSO = pRenderer->GetPipelineState(computeDesc, "Scene HZB Init PSO");

        computeDesc.CS = pRenderer->GetShader("HiZBuffer.hlsl", "BuildHZBCS", RHI::ERHIShaderType::CS);
        m_pDepthMipFilterPSO = pRenderer->GetPipelineState(computeDesc, "HZB Generate Mips PSO");

        computeDesc.CS = pRenderer->GetShader("HiZBuffer.hlsl", "BuildHZBCS", RHI::ERHIShaderType::CS, { "MIN_MAX_FILTER=1" });
        m_pDepthMipFilterMinMaxPSO = pRenderer->GetPipelineState(computeDesc, "HZB Generate Mips MinMax PSO");
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

        auto reprojectionPass = rg->AddPass<FDepthReprojectionData>("Depth Reprojection", RG::RenderPassType::Compute,
            [&](FDepthReprojectionData& data, RG::RGBuilder& builder)
            {
                data.PrevDepth = builder.Read(m_pRenderer->GetPrevSceneDepthHandle());

                RHI::RHITextureDesc texDesc {};
                texDesc.Width = m_HZBSize.x;
                texDesc.Height = m_HZBSize.y;
                texDesc.Format = RHI::ERHIFormat::R16F;
                texDesc.Usage = RHI::RHITextureUsageUnorderedAccess;

                data.ReprojectedDepth = builder.Create<RG::RGTexture>(texDesc, "RT_ReprojectedDepth");
                data.ReprojectedDepth = builder.Write(data.ReprojectedDepth);
            },
            [=](const FDepthReprojectionData& data, RHI::RHICommandList* pCmdList)
            {
                ReprojectDepth(pCmdList, rg->GetTexture(data.ReprojectedDepth));
            });

        struct FDepthDilationData
        {
            RG::RGHandle ReprojectedDepth;
            RG::RGHandle DilatedDepth;
        };

        RG::RGHandle hzb;

        auto dilationPass = rg->AddPass<FDepthDilationData>("Depth Dilation", RG::RenderPassType::Compute,
            [&](FDepthDilationData& data, RG::RGBuilder& builder)
            {
                data.ReprojectedDepth = builder.Read(reprojectionPass->ReprojectedDepth);

                RHI::RHITextureDesc texDesc {};
                texDesc.Width = m_HZBSize.x;
                texDesc.Height = m_HZBSize.y;
                texDesc.MipLevels = m_HZBMipCount;
                texDesc.Format = RHI::ERHIFormat::R16F;
                texDesc.Usage = RHI::RHITextureUsageUnorderedAccess;

                hzb = builder.Create<RG::RGTexture>(texDesc, "RT_1stPhaseHZB");

                data.DilatedDepth = builder.Write(hzb);
            },
            [=](const FDepthDilationData& data, RHI::RHICommandList* pCmdList)
            {
                DilationDepth(pCmdList, 
                    rg->GetTexture(data.ReprojectedDepth), 
                    rg->GetTexture(data.DilatedDepth));
            });

        struct FBuildHZBData
        {
            RG::RGHandle HZB;
        };

        rg->AddPass<FBuildHZBData>("Build HZB", RG::RenderPassType::Compute,
            [&](FBuildHZBData& data, RG::RGBuilder& builder)
            {
                data.HZB = builder.Read(dilationPass->DilatedDepth);

                m_CullingHZBMips1stPhase[0] = data.HZB;
                for (uint32_t i = 1; i < m_HZBMipCount; ++i)
                {
                    m_CullingHZBMips1stPhase[i] = builder.Write(hzb, i);
                }
            },
            [=](const FBuildHZBData& data, RHI::RHICommandList* pCmdList)
            {
                RG::RGTexture* pHZB = rg->GetTexture(data.HZB);
                BuildHZB(pCmdList, pHZB);
            });
    }

    void HiZBuffer::GenerateCullingHZB2ndPhase(RG::RenderGraph *rg, RG::RGHandle depthRT)
    {
        RENDER_GRAPH_EVENT(rg, "HiZBuffer::GenerateCullingHZB2ndPhase");

        struct FInitHZBData
        {
            RG::RGHandle InputDepthRT;
            RG::RGHandle HZB;
        };

        RG::RGHandle hzb;

        auto initPass = rg->AddPass<FInitHZBData>("Init HZB", RG::RenderPassType::Compute,
            [&](FInitHZBData& data, RG::RGBuilder& builder)
            {
                data.InputDepthRT = builder.Read(depthRT);

                RHI::RHITextureDesc texDesc {};
                texDesc.Width = m_HZBSize.x;
                texDesc.Height = m_HZBSize.y;
                texDesc.MipLevels = m_HZBMipCount;
                texDesc.Format = RHI::ERHIFormat::R16F;
                texDesc.Usage = RHI::RHITextureUsageUnorderedAccess;

                hzb = builder.Create<RG::RGTexture>(texDesc, "RT_2ndPhaseHZB");

                data.HZB = builder.Write(hzb);
            },
            [=](const FInitHZBData& data, RHI::RHICommandList* pCmdList)
            {
                InitHZB(pCmdList, 
                    rg->GetTexture(data.InputDepthRT), 
                    rg->GetTexture(data.HZB));
            });

        struct FBuildHZBData
        {
            RG::RGHandle HZB;
        };

        rg->AddPass<FBuildHZBData>("Build HZB", RG::RenderPassType::Compute,
            [&](FBuildHZBData& data, RG::RGBuilder& builder)
            {
                data.HZB = builder.Read(initPass->HZB);

                m_CullingHZBMips2ndPhase[0] = data.HZB;
                for (uint32_t i = 1; i < m_HZBMipCount; ++i)
                {
                    m_CullingHZBMips2ndPhase[i] = builder.Write(hzb, i);
                }
            },
            [=](const FBuildHZBData& data, RHI::RHICommandList* pCmdList)
            {
                RG::RGTexture* pHZB = rg->GetTexture(data.HZB);
                BuildHZB(pCmdList, pHZB);
            });
    }

    void HiZBuffer::GenerateSceneHZB(RG::RenderGraph *rg, RG::RGHandle depthRT)
    {
        RENDER_GRAPH_EVENT(rg, "HiZBuffer::GenerateSceneHZB");

        struct FInitHZBData
        {
            RG::RGHandle InputDepthRT;
            RG::RGHandle HZB;
        };

        RG::RGHandle hzb;

        auto initPass = rg->AddPass<FInitHZBData>("Init Scene HZB", RG::RenderPassType::Compute,
            [&](FInitHZBData& data, RG::RGBuilder& builder)
            {
                data.InputDepthRT = builder.Read(depthRT);

                RHI::RHITextureDesc texDesc {};
                texDesc.Width = m_HZBSize.x;
                texDesc.Height = m_HZBSize.y;
                texDesc.MipLevels = m_HZBMipCount;
                texDesc.Format = RHI::ERHIFormat::R16F;
                texDesc.Usage = RHI::RHITextureUsageUnorderedAccess;

                hzb = builder.Create<RG::RGTexture>(texDesc, "RT_SceneHZB");

                data.HZB = builder.Write(hzb);
            },
            [=](const FInitHZBData& data, RHI::RHICommandList* pCmdList)
            {
                InitHZB(pCmdList, 
                    rg->GetTexture(data.InputDepthRT), 
                    rg->GetTexture(data.HZB),
                    true);
            });

        struct FBuildHZBData
        {
            RG::RGHandle HZB;
        };

        rg->AddPass<FBuildHZBData>("Build Scene HZB", RG::RenderPassType::Compute,
            [&](FBuildHZBData& data, RG::RGBuilder& builder)
            {
                data.HZB = builder.Read(initPass->HZB);

                m_SceneHZBMips[0] = data.HZB;
                for (uint32_t i = 1; i < m_HZBMipCount; ++i)
                {
                    m_SceneHZBMips[i] = builder.Write(hzb, i);
                }
            },
            [=](const FBuildHZBData& data, RHI::RHICommandList* pCmdList)
            {
                RG::RGTexture* pHZB = rg->GetTexture(data.HZB);
                BuildHZB(pCmdList, pHZB, true);
            });
    }

    RG::RGHandle HiZBuffer::GetCullingHZBMip1stPhase(uint32_t mip) const
    {
        assert(mip < m_HZBMipCount);
        return m_CullingHZBMips1stPhase[mip];
    }

    RG::RGHandle HiZBuffer::GetCullingHZBMip2ndPhase(uint32_t mip) const
    {
        assert(mip < m_HZBMipCount);
        return m_CullingHZBMips2ndPhase[mip];
    }

    RG::RGHandle HiZBuffer::GetSceneHZBMip(uint32_t mip) const
    {
        assert(mip < m_HZBMipCount);
        return m_SceneHZBMips[mip];
    }

    void HiZBuffer::CalcHZBSize()
    {
        uint32_t mipsX = (uint32_t)max(ceilf(log2f((float)m_pRenderer->GetRenderWidth())), 1.0f);
        uint32_t mipsY = (uint32_t)max(ceilf(log2f((float)m_pRenderer->GetRenderHeight())), 1.0f);

        m_HZBMipCount = max(mipsX, mipsY);
        assert(m_HZBMipCount <= MAX_HZB_MIP_COUNT);

        m_HZBSize.x = 1 << (mipsX - 1);
        m_HZBSize.y = 1 << (mipsY - 1);
    }

    void HiZBuffer::ReprojectDepth(RHI::RHICommandList* pCmdList, RG::RGTexture* reprojectedDepthTexture)
    {
        float clearValue[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        pCmdList->ClearUAV(reprojectedDepthTexture->GetTexture(), reprojectedDepthTexture->GetUAV(), clearValue);
        pCmdList->TextureBarrier(reprojectedDepthTexture->GetTexture(), RHI::RHI_ALL_SUB_RESOURCE,
                                 RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);

        pCmdList->SetPipelineState(m_pDepthReprojectionPSO);

        uint32_t rootConsts[4] = {
            0,
            reprojectedDepthTexture->GetUAV()->GetHeapIndex(),
            m_HZBSize.x,
            m_HZBSize.y
        };
        pCmdList->SetComputeConstants(0, rootConsts, sizeof(rootConsts));

        pCmdList->Dispatch(DivideRoundingUp(m_HZBSize.x, 8), DivideRoundingUp(m_HZBSize.y, 8), 1);
    }

    void HiZBuffer::DilationDepth(RHI::RHICommandList* pCmdList, RG::RGTexture* reprojectedDepthSRV, RG::RGTexture* hzbMip0UAV)
    {
        pCmdList->SetPipelineState(m_pDepthDilationPSO);

        uint32_t rootConsts[4] = {
            reprojectedDepthSRV->GetSRV()->GetHeapIndex(),
            hzbMip0UAV->GetUAV()->GetHeapIndex(),
            m_HZBSize.x,
            m_HZBSize.y
        };
        pCmdList->SetComputeConstants(0, rootConsts, sizeof(rootConsts));

        pCmdList->Dispatch(DivideRoundingUp(m_HZBSize.x, 8), DivideRoundingUp(m_HZBSize.y, 8), 1);
    }

    void HiZBuffer::BuildHZB(RHI::RHICommandList* pCmdList, RG::RGTexture* texture, bool minMax)
    {
        pCmdList->SetPipelineState(minMax ? m_pDepthMipFilterMinMaxPSO : m_pDepthMipFilterPSO);

        const RHI::RHITextureDesc &textureDesc = texture->GetTexture()->GetDesc();

        varAU2(dispatchThreadGroupCountXY);
        varAU2(workGroupOffset);
        varAU2(numWorkGroupsAndMips);
        varAU4(rectInfo) = initAU4(0, 0, textureDesc.Width, textureDesc.Height);
        SpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo, textureDesc.MipLevels - 1);

        struct SPDConstants
        {
            uint32_t Mips;
            uint32_t NumWorkGroups;
            uint2 WorkGroupOffset;

            float2 InvInputSize;
            uint32_t ImgSrc;
            uint32_t SpdGlobalAtomicUAV;

            uint4 ImgDst[12];
        };

        SPDConstants constants = {};
        constants.NumWorkGroups = numWorkGroupsAndMips[0];
        constants.Mips = numWorkGroupsAndMips[1];
        constants.WorkGroupOffset[0] = workGroupOffset[0];
        constants.WorkGroupOffset[1] = workGroupOffset[1];
        constants.InvInputSize[0] = 1.0f / textureDesc.Width;
        constants.InvInputSize[1] = 1.0f / textureDesc.Height;

        constants.ImgSrc = texture->GetSRV()->GetHeapIndex();
        constants.SpdGlobalAtomicUAV = m_pRenderer->GetSPDCounterBuffer()->GetUAV()->GetHeapIndex();

        for (uint32_t i = 0; i < textureDesc.MipLevels - 1; ++i)
        {
            constants.ImgDst[i].x = texture->GetUAV(i + 1, 0)->GetHeapIndex();
        }

        pCmdList->SetComputeConstants(1, &constants, sizeof(constants));

        uint32_t dispatchX = dispatchThreadGroupCountXY[0];
        uint32_t dispatchY = dispatchThreadGroupCountXY[1];
        pCmdList->Dispatch(dispatchX, dispatchY, 1);
    }

    void HiZBuffer::InitHZB(RHI::RHICommandList* pCmdList, RG::RGTexture* inputDepthSRV, RG::RGTexture* hzbMip0UAV, bool minMax)
    {
        pCmdList->SetPipelineState(minMax ? m_pInitSceneHZBPSO : m_pInitHZBPSO);

        uint32_t rootConsts[4] = {
            inputDepthSRV->GetSRV()->GetHeapIndex(),
            hzbMip0UAV->GetUAV()->GetHeapIndex(),
            m_HZBSize.x,
            m_HZBSize.y
        };
        pCmdList->SetComputeConstants(0, rootConsts, sizeof(rootConsts));

        pCmdList->Dispatch(DivideRoundingUp(m_HZBSize.x, 8), DivideRoundingUp(m_HZBSize.y, 8), 1);
    }
}