#include "RHIPipelineStateVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIGraphicsPipelineStateVK::RHIGraphicsPipelineStateVK(RHIDeviceVK *device, const RHIGraphicsPipelineStateDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
        mType = ERHIPipelineType::Graphics;
    }

    RHIGraphicsPipelineStateVK::~RHIGraphicsPipelineStateVK()
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mPipeline);
    }

    bool RHIGraphicsPipelineStateVK::Create()
    {
        return true;
    }

    RHIComputePipelineStateVK::RHIComputePipelineStateVK(RHIDeviceVK *device, const RHIComputePipelineStateDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
        mType = ERHIPipelineType::Compute;
    }

    RHIComputePipelineStateVK::~RHIComputePipelineStateVK()
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mPipeline);
    }

    bool RHIComputePipelineStateVK::Create()
    {
        return true;
    }
}