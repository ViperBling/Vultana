#pragma once

#include "RHI/RHI.hpp"

#include "Utilities/Hash.hpp"

#include <EASTL/hash_map.h>
#include <EASTL/unique_ptr.h>

inline uint64_t HashCombine64(uint64_t hash0, uint64_t hash1)
{
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (hash1 ^ hash0) * kMul;
    a ^= (a >> 47);
    uint64_t b = (hash0 ^ a) * kMul;
    b ^= (b >> 47);
    return b * kMul;
}

namespace eastl
{
    template<>
    struct hash<RHI::RHIGraphicsPipelineStateDesc>
    {
        size_t operator()(const RHI::RHIGraphicsPipelineStateDesc& desc) const
        {
            uint64_t vsHash = desc.VS->GetHash();
            uint64_t psHash = desc.PS ? desc.PS->GetHash() : 0;

            const size_t stateOffset = offsetof(RHI::RHIGraphicsPipelineStateDesc, RasterizerState);
            uint64_t stateHash = CityHash64(reinterpret_cast<const char*>(&desc) + stateOffset, sizeof(RHI::RHIGraphicsPipelineStateDesc) - stateOffset);
            uint64_t hash = HashCombine64(HashCombine64(vsHash, psHash), stateOffset);

            static_assert(sizeof(size_t) == sizeof(uint64_t), "Only supports 64-bit platforms");
            return hash;
        }
    };

    template<>
    struct hash<RHI::RHIMeshShadingPipelineStateDesc>
    {
        size_t operator()(const RHI::RHIMeshShadingPipelineStateDesc& desc) const
        {
            uint64_t asHash = desc.AS ? desc.AS->GetHash() : 0;
            uint64_t m_sHash = desc.MS->GetHash();
            uint64_t psHash = desc.PS ? desc.PS->GetHash() : 0;

            const size_t stateOffset = offsetof(RHI::RHIMeshShadingPipelineStateDesc, RasterizerState);
            uint64_t stateHash = CityHash64(reinterpret_cast<const char*>(&desc) + stateOffset, sizeof(RHI::RHIMeshShadingPipelineStateDesc) - stateOffset);
            uint64_t hash = HashCombine64(HashCombine64(HashCombine64(asHash, m_sHash), psHash), stateHash);

            static_assert(sizeof(size_t) == sizeof(uint64_t), "Only supports 64-bit platforms");
            return hash;
        }
    };

    template<>
    struct hash<RHI::RHIComputePipelineStateDesc>
    {
        size_t operator()(const RHI::RHIComputePipelineStateDesc& desc) const
        {
            static_assert(sizeof(size_t) == sizeof(uint64_t), "Only supports 64-bit platforms");
            return desc.CS->GetHash();
        }
    };
}

namespace Renderer
{
    class RendererBase;

    class PipelineStateCache
    {
    public:
        PipelineStateCache(RendererBase* renderer);

        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc& desc, const eastl::string& name);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIMeshShadingPipelineStateDesc& desc, const eastl::string& name);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIComputePipelineStateDesc& desc, const eastl::string& name);

        void RecreatePSO(RHI::RHIShader* shader);
    
    private:
        RendererBase* m_pRenderer = nullptr;
        eastl::hash_map<RHI::RHIGraphicsPipelineStateDesc, eastl::unique_ptr<RHI::RHIPipelineState>> m_CachedGraphicsPSO;
        eastl::hash_map<RHI::RHIMeshShadingPipelineStateDesc, eastl::unique_ptr<RHI::RHIPipelineState>> m_CachedMeshletPSO;
        eastl::hash_map<RHI::RHIComputePipelineStateDesc, eastl::unique_ptr<RHI::RHIPipelineState>> m_CachedComputePSO;
    };
}