#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIPipelineState.hpp"

namespace RHI
{
    class FVulkanDevice;

    class FVulkanGraphicsPipelineState : public FRHIPipelineState
    {
    public:
        FVulkanGraphicsPipelineState(FVulkanDevice *device, const FRHIGraphicsPipelineStateDesc &desc, const eastl::string &name);
        ~FVulkanGraphicsPipelineState();

        virtual void* GetNativeHandle() const override { return m_Pipeline; }
        virtual bool Create() override;

    private:
        FRHIGraphicsPipelineStateDesc m_Desc;
        vk::Pipeline m_Pipeline = VK_NULL_HANDLE;
    };

    class FVulkanMeshShadingPipelineState : public FRHIPipelineState
    {
    public:
        FVulkanMeshShadingPipelineState(FVulkanDevice *device, const FRHIMeshShadingPipelineStateDesc &desc, const eastl::string &name);
        ~FVulkanMeshShadingPipelineState();

        virtual void* GetNativeHandle() const override { return m_Pipeline; }
        virtual bool Create() override;

    private:
        FRHIMeshShadingPipelineStateDesc m_Desc;
        vk::Pipeline m_Pipeline = VK_NULL_HANDLE;
    };

    class FVulkanComputePipelineState : public FRHIPipelineState
    {
    public:
        FVulkanComputePipelineState(FVulkanDevice *device, const FRHIComputePipelineStateDesc &desc, const eastl::string &name);
        ~FVulkanComputePipelineState();

        virtual void* GetNativeHandle() const override { return m_Pipeline; }
        virtual bool Create() override;

    private:
        FRHIComputePipelineStateDesc m_Desc;
        vk::Pipeline m_Pipeline = VK_NULL_HANDLE;
    };
}