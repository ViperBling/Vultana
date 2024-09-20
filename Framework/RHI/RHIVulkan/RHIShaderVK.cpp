#include "RHIShaderVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIShaderVK::RHIShaderVK(RHIDeviceVK *device, const RHIShaderDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
    }

    RHIShaderVK::~RHIShaderVK()
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mShaderModule);
    }

    bool RHIShaderVK::Create(tcb::span<uint8_t> data)
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mShaderModule);

        auto device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();

        vk::ShaderModuleCreateInfo shaderCI {};
        shaderCI.setCodeSize(data.size());
        shaderCI.setPCode(reinterpret_cast<const uint32_t*>(data.data()));

        mShaderModule = device.createShaderModule(shaderCI, nullptr, dynamicLoader);
        if (!mShaderModule)
        {
            VTNA_LOG_ERROR("[RHIShaderVK] Failed to create {}", mName);
            return false;
        }
        SetDebugName(device, vk::ObjectType::eShaderModule, mShaderModule, mName.c_str(), dynamicLoader);
        mHash = CityHash64(reinterpret_cast<const char*>(data.data()), data.size());

        return true;
    }
}