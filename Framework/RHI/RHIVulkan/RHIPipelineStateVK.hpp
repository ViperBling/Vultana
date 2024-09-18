#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIPipelineState.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHIGraphicsPipelineStateVK : public RHIPipelineState
    {
    public:
        RHIGraphicsPipelineStateVK(RHIDeviceVK *device, const RHIGraphicsPipelineStateDesc &desc, const std::string &name);
        ~RHIGraphicsPipelineStateVK();

        virtual void* GetNativeHandle() const override { return mPipeline; }
        virtual bool Create() override;

    private:
        RHIGraphicsPipelineStateDesc mDesc;
        vk::Pipeline mPipeline = VK_NULL_HANDLE;
    };

    class RHIComputePipelineStateVK : public RHIPipelineState
    {
    public:
        RHIComputePipelineStateVK(RHIDeviceVK *device, const RHIComputePipelineStateDesc &desc, const std::string &name);
        ~RHIComputePipelineStateVK();

        virtual void* GetNativeHandle() const override { return mPipeline; }
        virtual bool Create() override;

    private:
        RHIComputePipelineStateDesc mDesc;
        vk::Pipeline mPipeline = VK_NULL_HANDLE;
    };
}