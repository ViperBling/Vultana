#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    struct SamplerCreateInfo
    {
        RHISamplerAddressMode AddressModeU = RHISamplerAddressMode::Clamp;
        RHISamplerAddressMode AddressModeV = RHISamplerAddressMode::Clamp;
        RHISamplerAddressMode AddressModeW = RHISamplerAddressMode::Clamp;
        RHISamplerFilterMode MagFilter = RHISamplerFilterMode::Nearest;
        RHISamplerFilterMode MinFilter = RHISamplerFilterMode::Nearest;
        RHISamplerFilterMode MipmapMode = RHISamplerFilterMode::Nearest;
        float LodMinClamp = 0;
        float LodMaxClamp = 0;
        RHICompareOp CompareOp = RHICompareOp::Never;
        uint8_t MaxAnisotropy = 1;
        std::string Name;
    };

    class RHISampler
    {
    public:
        NOCOPY(RHISampler)
        virtual ~RHISampler() = default;

        virtual void Destroy() = 0;

    protected:
        explicit RHISampler(const SamplerCreateInfo& createInfo) {}
    };
}