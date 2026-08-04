#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIShader.hpp"

namespace RHI
{
    class FVulkanDevice;

    class FVulkanShader : public FRHIShader
    {
    public:
        FVulkanShader(FVulkanDevice* device, const FRHIShaderDesc& desc, const eastl::string& name);
        ~FVulkanShader();

        virtual bool Create(eastl::span<uint8_t> data) override;
        virtual void* GetNativeHandle() const override { return m_ShaderModule; }

    private:
        vk::ShaderModule m_ShaderModule;
    };
}
