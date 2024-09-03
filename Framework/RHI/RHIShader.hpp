#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIShader : public RHIResource
    {
    public:
        virtual ~RHIShader() = default;

        const RHIShaderDesc& GetDesc() const { return mDesc; }
        uint64_t GetHash() const { return mHash; }

        virtual bool Create(tcb::span<uint8_t> data) = 0;

    protected:
        RHIShaderDesc mDesc {};
        uint64_t mHash;
    };
}