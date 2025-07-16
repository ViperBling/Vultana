#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIBuffer : public RHIResource
    {
    public:
        const RHIBufferDesc& GetDesc() const { return m_Desc; }
        virtual bool IsBuffer() const override { return true; }
        
        virtual void* GetCPUAddress() = 0;
        virtual uint64_t GetGPUAddress() = 0;
        virtual uint32_t GetRequiredStagingBufferSize() const = 0;

    protected:
        RHIBufferDesc m_Desc {};
    };
}