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

    inline bool operator==(const RHI::RHIMeshShadingPipelineStateDesc& lhs, const RHI::RHIMeshShadingPipelineStateDesc& rhs)
    {
        if (lhs.MS->GetHash() != rhs.MS->GetHash()) return false;

        uint64_t lhsPSHash = lhs.PS ? lhs.PS->GetHash() : 0;
        uint64_t rhsPSHash = rhs.PS ? rhs.PS->GetHash() : 0;
        if (lhsPSHash != rhsPSHash) return false;

        uint64_t lhsASHash = lhs.AS ? lhs.AS->GetHash() : 0;
        uint64_t rhsASHash = rhs.AS ? rhs.AS->GetHash() : 0;
        if (lhsASHash != rhsASHash) return false;

        const size_t stateOffset = offsetof(RHI::RHIMeshShadingPipelineStateDesc, RasterizerState);
        void* lhsStates = (char*)&lhs + stateOffset;
        void* rhsStates = (char*)&rhs + stateOffset;

        return memcmp(lhsStates, rhsStates, sizeof(RHI::RHIMeshShadingPipelineStateDesc) - stateOffset) == 0;
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
        m_pRenderer = renderer;
    }

    RHI::RHIPipelineState *PipelineStateCache::GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc &desc, const eastl::string &name)
    {
        auto iter = m_CachedGraphicsPSO.find(desc);
        if (iter != m_CachedGraphicsPSO.end())
        {
            return iter->second.get();
        }

        RHI::RHIPipelineState* pPSO = m_pRenderer->GetDevice()->CreateGraphicsPipelineState(desc, name);
        if (pPSO != nullptr)
        {
            m_CachedGraphicsPSO.insert(eastl::make_pair(desc, eastl::unique_ptr<RHI::RHIPipelineState>(pPSO)));
        }
        return pPSO;
    }

    RHI::RHIPipelineState* PipelineStateCache::GetPipelineState(const RHI::RHIMeshShadingPipelineStateDesc& desc, const eastl::string& name)
    {
        auto iter = m_CachedMeshletPSO.find(desc);
        if (iter != m_CachedMeshletPSO.end())
        {
            return iter->second.get();
        }

        RHI::RHIPipelineState* pPSO = m_pRenderer->GetDevice()->CreateMeshShadingPipelineState(desc, name);
        if (pPSO != nullptr)
        {
            m_CachedMeshletPSO.insert(eastl::make_pair(desc, eastl::unique_ptr<RHI::RHIPipelineState>(pPSO)));
        }
        return pPSO;
    }

    RHI::RHIPipelineState *PipelineStateCache::GetPipelineState(const RHI::RHIComputePipelineStateDesc &desc, const eastl::string &name)
    {
        auto iter = m_CachedComputePSO.find(desc);
        if (iter != m_CachedComputePSO.end())
        {
            return iter->second.get();
        }

        RHI::RHIPipelineState* pPSO = m_pRenderer->GetDevice()->CreateComputePipelineState(desc, name);
        if (pPSO)
        {
            m_CachedComputePSO.insert(eastl::make_pair(desc, eastl::unique_ptr<RHI::RHIPipelineState>(pPSO)));
        }

        return pPSO;
    }

    void PipelineStateCache::RecreatePSO(RHI::RHIShader *shader)
    {
        for (auto iter = m_CachedGraphicsPSO.begin(); iter != m_CachedGraphicsPSO.end(); iter++)
        {
            const RHI::RHIGraphicsPipelineStateDesc& desc = iter->first;
            RHI::RHIPipelineState* pPSO = iter->second.get();
            if (desc.VS == shader || desc.PS == shader)
            {
                pPSO->Create();
            }
        }
        for (auto iter = m_CachedMeshletPSO.begin(); iter != m_CachedMeshletPSO.end(); iter++)
        {
            const RHI::RHIMeshShadingPipelineStateDesc& desc = iter->first;
            RHI::RHIPipelineState* pPSO = iter->second.get();
            if (desc.AS == shader || desc.MS == shader || desc.PS == shader)
            {
                pPSO->Create();
            }
        }
        for (auto iter = m_CachedComputePSO.begin(); iter != m_CachedComputePSO.end(); iter++)
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