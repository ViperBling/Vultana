#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIPipelineState.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHIGraphicsPipelineStateVK : public RHIPipelineState
    {
    public:
        RHIGraphicsPipelineStateVK(RHIDeviceVK *device, const RHIGraphicsPipelineStateDesc &desc, const eastl::string &name);
        ~RHIGraphicsPipelineStateVK();

        virtual void* GetNativeHandle() const override { return mPipeline; }
        virtual bool Create() override;

    private:
        RHIGraphicsPipelineStateDesc mDesc;
        vk::Pipeline mPipeline = VK_NULL_HANDLE;
    };

    class RHIMeshShadingPipelineStateVK : public RHIPipelineState
    {
    public:
        RHIMeshShadingPipelineStateVK(RHIDeviceVK *device, const RHIMeshShadingPipelineStateDesc &desc, const eastl::string &name);
        ~RHIMeshShadingPipelineStateVK();

        virtual void* GetNativeHandle() const override { return mPipeline; }
        virtual bool Create() override;

    private:
        RHIMeshShadingPipelineStateDesc mDesc;
        vk::Pipeline mPipeline = VK_NULL_HANDLE;
    };

    class RHIComputePipelineStateVK : public RHIPipelineState
    {
    public:
        RHIComputePipelineStateVK(RHIDeviceVK *device, const RHIComputePipelineStateDesc &desc, const eastl::string &name);
        ~RHIComputePipelineStateVK();

        virtual void* GetNativeHandle() const override { return mPipeline; }
        virtual bool Create() override;

    private:
        RHIComputePipelineStateDesc mDesc;
        vk::Pipeline mPipeline = VK_NULL_HANDLE;
    };
}