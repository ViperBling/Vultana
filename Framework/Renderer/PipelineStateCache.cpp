#include "PipelineStateCache.hpp"
#include "RendererBase.hpp"

namespace RHI
{
    inline bool operator==(const RHI::RHIGraphicsPipelineStateDesc& lhs, const RHI::RHIGraphicsPipelineStateDesc& rhs)
    {
        if (lhs.VS->GetHash() != rhs.VS->GetHash()) return false;
        uint64_t lhsPSHash = lhs.PS ? lhs.PS->GetHash() : 0;
        uint64_t rhsPSHash = rhs.PS ? rhs.PS->GetHash() : 0;

        if (lhsPSHash != rhsPSHash) return false;

        const size_t stateOffset = offsetof(RHI::RHIGraphicsPipelineStateDesc, RasterizerState);
        void* lhsStates = (char*)&lhs + stateOffset;
        void* rhsStates = (char*)&rhs + stateOffset;

        return memcmp(lhsStates, rhsStates, sizeof(RHI::RHIGraphicsPipelineStateDesc) - stateOffset) == 0;
    }

    inline bool operator==(const RHI::RHIComputePipelineStateDesc& lhs, const RHI::RHIComputePipelineStateDesc& rhs)
    {
        return lhs.CS->GetHash() == rhs.CS->GetHash();
    }
}

namespace Renderer
{
    PipelineStateCache::PipelineStateCache(RendererBase *renderer)
    {
        mpRenderer = renderer;
    }

    RHI::RHIPipelineState *PipelineStateCache::GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc &desc, const std::string &name)
    {
        auto iter = mCachedGraphicsPSO.find(desc);
        if (iter != mCachedGraphicsPSO.end())
        {
            return iter->second.get();
        }

        RHI::RHIPipelineState* pPSO = mpRenderer->GetDevice()->CreateGraphicsPipelineState(desc, name);
        if (pPSO != nullptr)
        {
            mCachedGraphicsPSO.insert(std::make_pair(desc, std::unique_ptr<RHI::RHIPipelineState>(pPSO)));
        }
        return pPSO;
    }

    RHI::RHIPipelineState *PipelineStateCache::GetPipelineState(const RHI::RHIComputePipelineStateDesc &desc, const std::string &name)
    {
        auto iter = mCachedComputePSO.find(desc);
        if (iter != mCachedComputePSO.end())
        {
            return iter->second.get();
        }

        RHI::RHIPipelineState* pPSO = mpRenderer->GetDevice()->CreateComputePipelineState(desc, name);
        if (pPSO)
        {
            mCachedComputePSO.insert(std::make_pair(desc, std::unique_ptr<RHI::RHIPipelineState>(pPSO)));
        }

        return pPSO;
    }

    void PipelineStateCache::RecreatePSO(RHI::RHIShader *shader)
    {
        for (auto iter = mCachedGraphicsPSO.begin(); iter != mCachedGraphicsPSO.end(); iter++)
        {
            const RHI::RHIGraphicsPipelineStateDesc& desc = iter->first;
            RHI::RHIPipelineState* pPSO = iter->second.get();
            if (desc.VS == shader || desc.PS == shader)
            {
                pPSO->Create();
            }
        }
        for (auto iter = mCachedComputePSO.begin(); iter != mCachedComputePSO.end(); iter++)
        {
            const RHI::RHIComputePipelineStateDesc& desc = iter->first;
            RHI::RHIPipelineState* pPSO = iter->second.get();
            if (desc.CS == shader)
            {
                pPSO->Create();
            }
        }
    }
}