#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHIShader : public RHIResource
    {
    public:
        virtual ~RHIShader() = default;

        const RHIShaderDesc& GetDesc() const { return m_Desc; }
        uint64_t GetHash() const { return m_Hash; }

        virtual bool Create(eastl::span<uint8_t> data) = 0;

    protected:
        RHIShaderDesc m_Desc {};
        uint64_t m_Hash;
    };
}