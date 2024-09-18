#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIBuffer : public RHIResource
    {
    public:
        const RHIBufferDesc& GetDesc() const { return mDesc; }
        virtual bool IsBuffer() const override { return true; }
        
        virtual void* GetCPUAddress() const = 0;
        virtual uint64_t GetGPUAddress() const = 0;
        virtual uint32_t GetRequiredStagingBufferSize() const = 0;

    protected:
        RHIBufferDesc mDesc {};
    };
}