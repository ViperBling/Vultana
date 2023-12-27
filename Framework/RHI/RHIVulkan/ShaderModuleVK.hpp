#pragma once

#include "RHI/RHIShaderModule.hpp"

#include <vulkan/vulkan.hpp>
#include <string>
#include <unordered_map>

namespace Vultana
{
    class DeviceVK;

    class ShaderModuleVK : public RHIShaderModule
    {
    public:
        NOCOPY(ShaderModuleVK);
        ShaderModuleVK(DeviceVK &device, const ShaderModuleCreateInfo &createInfo);
        ~ShaderModuleVK();
        void Destroy() override;

        vk::ShaderModule GetVkShaderModule() const { return mShaderModule; }

        void BuildReflection(const ShaderModuleCreateInfo& createInfo);

        using ShaderInputLocationTable = std::unordered_map<std::string, uint32_t>;
        const ShaderInputLocationTable& GetLocationTable() const { return mInputLocationTable; }

    private:
        void CreateNaitiveShaderModule(const ShaderModuleCreateInfo &createInfo);

    private:
        DeviceVK &mDevice;
        vk::ShaderModule mShaderModule;
        ShaderInputLocationTable mInputLocationTable;
    };
}