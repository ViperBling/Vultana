#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHITexture : public RHIResource
    {
    public:
        virtual ~RHITexture() = default;

        const RHITextureDesc& GetDesc() const { return mDesc; }

        virtual bool IsTexture() const override { return true; }
        virtual uint32_t GetRequiredStagingBufferSize() const = 0;

    protected:
        RHITextureDesc mDesc {};
    };
}