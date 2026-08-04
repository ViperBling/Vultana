#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class FRHITexture : public FRHIResource
    {
    public:
        virtual ~FRHITexture() = default;

        const FRHITextureDesc& GetDesc() const { return m_Desc; }

        virtual bool IsTexture() const override { return true; }
        virtual uint32_t GetRequiredStagingBufferSize() const = 0;
        virtual uint32_t GetRowPitch(uint32_t mipLevel = 0) const = 0;

        virtual void* GetSharedHandle() const = 0;

    protected:
        FRHITextureDesc m_Desc {};
    };
}