#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIShader.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHIShaderVK : public RHIShader
    {
    public:
        RHIShaderVK(RHIDeviceVK* device, const RHIShaderDesc& desc, const eastl::string& name);
        ~RHIShaderVK();

        virtual bool Create(eastl::span<uint8_t> data) override;
        virtual void* GetNativeHandle() const override { return mShaderModule; }

    private:
        vk::ShaderModule mShaderModule;
    };
}
