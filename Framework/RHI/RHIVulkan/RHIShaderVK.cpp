#include "RHIShaderVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    FVulkanShader::FVulkanShader(FVulkanDevice *device, const FRHIShaderDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
    }

    FVulkanShader::~FVulkanShader()
    {
        ((FVulkanDevice*)m_pDevice)->Delete(m_ShaderModule);
    }

    bool FVulkanShader::Create(eastl::span<uint8_t> data)
    {
        ((FVulkanDevice*)m_pDevice)->Delete(m_ShaderModule);

        auto device = ((FVulkanDevice*)m_pDevice)->GetDevice();
        auto dynamicLoader = ((FVulkanDevice*)m_pDevice)->GetDynamicLoader();

        vk::ShaderModuleCreateInfo shaderCI {};
        shaderCI.setCodeSize(data.size());
        shaderCI.setPCode(reinterpret_cast<const uint32_t*>(data.data()));

        m_ShaderModule = device.createShaderModule(shaderCI, nullptr, dynamicLoader);
        if (!m_ShaderModule)
        {
            VTNA_LOG_ERROR("[RHIShaderVK] Failed to create {}", m_Name);
            return false;
        }
        SetDebugName(device, vk::ObjectType::eShaderModule, m_ShaderModule, m_Name.c_str(), dynamicLoader);
        m_Hash = CityHash64(reinterpret_cast<const char*>(data.data()), data.size());

        return true;
    }
}