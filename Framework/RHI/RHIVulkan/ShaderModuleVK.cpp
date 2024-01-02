#include "ShaderModuleVK.hpp"

#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"

#include <spirv_cross/spirv_cross.hpp>

namespace RHI
{
    ShaderModuleVK::ShaderModuleVK(DeviceVK &device, const ShaderModuleCreateInfo &createInfo)
        : RHIShaderModule(createInfo)
        , mDevice(device)
    {
        CreateNaitiveShaderModule(createInfo);
    }

    ShaderModuleVK::~ShaderModuleVK()
    {
        Destroy();
    }

    void ShaderModuleVK::Destroy()
    {
        if (mShaderModule)
        {
            mDevice.GetVkDevice().destroyShaderModule(mShaderModule);
            mShaderModule = VK_NULL_HANDLE;
        }
    }

    void ShaderModuleVK::BuildReflection(const ShaderModuleCreateInfo &createInfo)
    {
        spirv_cross::Compiler compiler(static_cast<const uint32_t*>(createInfo.Code), createInfo.CodeSize / sizeof(uint32_t));
        auto resource = compiler.get_shader_resources();
        for (auto& input : resource.stage_inputs)
        {
            auto location = compiler.get_decoration(input.id, spv::DecorationLocation);
            mInputLocationTable.emplace(input.name, location);
        }
    }

    void ShaderModuleVK::CreateNaitiveShaderModule(const ShaderModuleCreateInfo &createInfo)
    {
        vk::ShaderModuleCreateInfo ssCI {};
        ssCI.setPCode(static_cast<const uint32_t*>(createInfo.Code));
        ssCI.setCodeSize(createInfo.CodeSize);

        mShaderModule = mDevice.GetVkDevice().createShaderModule(ssCI);
        BuildReflection(createInfo);
    }
}