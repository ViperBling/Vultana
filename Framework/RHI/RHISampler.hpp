#pragma once

#include "RHICommon.hpp"

namespace RHI
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

        SamplerCreateInfo()
            : AddressModeU(RHISamplerAddressMode::Clamp)
            , AddressModeV(RHISamplerAddressMode::Clamp)
            , AddressModeW(RHISamplerAddressMode::Clamp)
            , MagFilter(RHISamplerFilterMode::Nearest)
            , MinFilter(RHISamplerFilterMode::Nearest)
            , MipmapMode(RHISamplerFilterMode::Nearest)
            , LodMinClamp(0)
            , LodMaxClamp(32.0f)
            , CompareOp(RHICompareOp::Never)
            , MaxAnisotropy(1)
            , Name("Sampler")
        {}
        SamplerCreateInfo& SetAddressModeU(RHISamplerAddressMode inAddressModeU)
        {
            AddressModeU = inAddressModeU;
            return *this;
        }
        SamplerCreateInfo& SetAddressModeV(RHISamplerAddressMode inAddressModeV)
        {
            AddressModeV = inAddressModeV;
            return *this;
        }
        SamplerCreateInfo& SetAddressModeW(RHISamplerAddressMode inAddressModeW)
        {
            AddressModeW = inAddressModeW;
            return *this;
        }
        SamplerCreateInfo& SetMagFilter(RHISamplerFilterMode inMagFilter)
        {
            MagFilter = inMagFilter;
            return *this;
        }
        SamplerCreateInfo& SetMinFilter(RHISamplerFilterMode inMinFilter)
        {
            MinFilter = inMinFilter;
            return *this;
        }
        SamplerCreateInfo& SetMipmapMode(RHISamplerFilterMode inMipmapMode)
        {
            MipmapMode = inMipmapMode;
            return *this;
        }
        SamplerCreateInfo& SetLodMinClamp(float inLodMinClamp)
        {
            LodMinClamp = inLodMinClamp;
            return *this;
        }
        SamplerCreateInfo& SetLodMaxClamp(float inLodMaxClamp)
        {
            LodMaxClamp = inLodMaxClamp;
            return *this;
        }
        SamplerCreateInfo& SetCompareOp(RHICompareOp inCompareOp)
        {
            CompareOp = inCompareOp;
            return *this;
        }
        SamplerCreateInfo& SetMaxAnisotropy(uint8_t inMaxAnisotropy)
        {
            MaxAnisotropy = inMaxAnisotropy;
            return *this;
        }
        SamplerCreateInfo& SetName(const std::string& inName)
        {
            Name = inName;
            return *this;
        }
    };

    class RHISampler
    {
    public:
        NOCOPY(RHISampler)
        virtual ~RHISampler() = default;

    protected:
        explicit RHISampler(const SamplerCreateInfo& createInfo) {}
    };
}