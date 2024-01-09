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

        virtual bool SetShaderData(const uint8_t* data, uint32_t size) = 0;

    protected:
        RHIShaderDesc mDesc {};
        uint64_t mHash;
    };
}