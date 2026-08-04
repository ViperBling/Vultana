#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class FRHIShader : public FRHIResource
    {
    public:
        virtual ~FRHIShader() = default;

        const FRHIShaderDesc& GetDesc() const { return m_Desc; }
        uint64_t GetHash() const { return m_Hash; }

        virtual bool Create(eastl::span<uint8_t> data) = 0;

    protected:
        FRHIShaderDesc m_Desc {};
        uint64_t m_Hash;
    };
}