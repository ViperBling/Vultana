#include "RHIPipelineStateVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIShaderVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIGraphicsPipelineStateVK::RHIGraphicsPipelineStateVK(RHIDeviceVK *device, const RHIGraphicsPipelineStateDesc &desc, const eastl::string &name)
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
        auto device = static_cast<RHIDeviceVK*>(mpDevice);
        device->Delete(mPipeline);

        vk::PipelineShaderStageCreateInfo shaderStages[2];
        shaderStages[0].setStage(vk::ShaderStageFlagBits::eVertex);
        shaderStages[0].setModule((VkShaderModule)mDesc.VS->GetNativeHandle());
        shaderStages[0].setPName(mDesc.VS->GetDesc().EntryPoint.c_str());
        if (mDesc.PS)
        {
            shaderStages[1].setStage(vk::ShaderStageFlagBits::eFragment);
            shaderStages[1].setModule((VkShaderModule)mDesc.PS->GetNativeHandle());
            shaderStages[1].setPName(mDesc.PS->GetDesc().EntryPoint.c_str());
        }

        vk::PipelineVertexInputStateCreateInfo viStateCI {};
        vk::PipelineInputAssemblyStateCreateInfo iaStateCI {};
        iaStateCI.topology = ToVKPrimitiveTopology(mDesc.PrimitiveType);

        vk::PipelineMultisampleStateCreateInfo msStateCI {};
        msStateCI.rasterizationSamples = vk::SampleCountFlagBits::e1;

        vk::DynamicState dynamicStates[4] = 
        {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
            vk::DynamicState::eBlendConstants,
            vk::DynamicState::eStencilReference
        };
        vk::PipelineDynamicStateCreateInfo dyStateCI {};
        dyStateCI.setDynamicStates(dynamicStates);

        vk::PipelineViewportStateCreateInfo vpStateCI {};
        vpStateCI.viewportCount = 1;
        vpStateCI.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rsStateCI = ToVKPipelineRSStateCreateInfo(mDesc.RasterizerState);
        vk::PipelineDepthStencilStateCreateInfo dsStateCI = ToVKPipelineDSStateCreateInfo(mDesc.DepthStencilState);

        vk::PipelineColorBlendAttachmentState blendAttachmentState[RHI_MAX_COLOR_ATTACHMENT_COUNT] {};
        vk::PipelineColorBlendStateCreateInfo cbStateCI = ToVKPipelineCBStateCreateInfo(mDesc.BlendState, blendAttachmentState);

        vk::Format colorFormats[RHI_MAX_COLOR_ATTACHMENT_COUNT];
        vk::PipelineRenderingCreateInfo renderingCI = ToVKPipelineRenderingCreateInfo(mDesc, colorFormats);

        vk::GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.setPNext(&renderingCI);
        pipelineCI.setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT);
        pipelineCI.setStageCount(mDesc.PS ? 2 : 1);
        pipelineCI.setPStages(shaderStages);
        pipelineCI.setPVertexInputState(&viStateCI);
        pipelineCI.setPInputAssemblyState(&iaStateCI);
        pipelineCI.setPViewportState(&vpStateCI);
        pipelineCI.setPRasterizationState(&rsStateCI);
        pipelineCI.setPMultisampleState(&msStateCI);
        pipelineCI.setPDepthStencilState(&dsStateCI);
        pipelineCI.setPColorBlendState(&cbStateCI);
        pipelineCI.setPDynamicState(&dyStateCI);
        pipelineCI.setLayout(device->GetPipelineLayout());

        auto deviceHandle = device->GetDevice();
        auto dynamicLoader = device->GetDynamicLoader();
        auto result = deviceHandle.createGraphicsPipelines(nullptr, 1, &pipelineCI, nullptr, &mPipeline, dynamicLoader);
        if (result != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHIGraphicsPipelineStateVK] Failed to create {}", mName);
            return false;
        }
        SetDebugName(deviceHandle, vk::ObjectType::ePipeline, (uint64_t)(VkPipeline)mPipeline, mName.c_str(), dynamicLoader);
        return true;
    }

    RHIMeshShadingPipelineStateVK::RHIMeshShadingPipelineStateVK(RHIDeviceVK *device, const RHIMeshShadingPipelineStateDesc &desc, const eastl::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
        mType = ERHIPipelineType::MeshShading;
    }
    
    RHIMeshShadingPipelineStateVK::~RHIMeshShadingPipelineStateVK()
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mPipeline);
    }

    bool RHIMeshShadingPipelineStateVK::Create()
    {
        auto device = static_cast<RHIDeviceVK*>(mpDevice);
        device->Delete(mPipeline);

        uint32_t shaderStageCount = 1;
        vk::PipelineShaderStageCreateInfo shaderStages[3];
        shaderStages[0].setStage(vk::ShaderStageFlagBits::eMeshEXT);
        shaderStages[0].setModule((VkShaderModule)mDesc.MS->GetNativeHandle());
        shaderStages[0].setPName(mDesc.MS->GetDesc().EntryPoint.c_str());

        if (mDesc.AS)
        {
            shaderStages[shaderStageCount].setStage(vk::ShaderStageFlagBits::eTaskEXT);
            shaderStages[shaderStageCount].setModule((VkShaderModule)mDesc.AS->GetNativeHandle());
            shaderStages[shaderStageCount].setPName(mDesc.AS->GetDesc().EntryPoint.c_str());
            shaderStageCount++;
        }
        if (mDesc.PS)
        {
            shaderStages[shaderStageCount].setStage(vk::ShaderStageFlagBits::eFragment);
            shaderStages[shaderStageCount].setModule((VkShaderModule)mDesc.PS->GetNativeHandle());
            shaderStages[shaderStageCount].setPName(mDesc.PS->GetDesc().EntryPoint.c_str());
            shaderStageCount++;
        }

        vk::PipelineMultisampleStateCreateInfo msStateCI {};
        msStateCI.rasterizationSamples = vk::SampleCountFlagBits::e1;

        vk::DynamicState dynamicStates[4] = 
        {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
            vk::DynamicState::eBlendConstants,
            vk::DynamicState::eStencilReference
        };
        vk::PipelineDynamicStateCreateInfo dyStateCI {};
        dyStateCI.setDynamicStates(dynamicStates);

        vk::PipelineViewportStateCreateInfo vpStateCI {};
        vpStateCI.viewportCount = 1;
        vpStateCI.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rsStateCI = ToVKPipelineRSStateCreateInfo(mDesc.RasterizerState);
        vk::PipelineDepthStencilStateCreateInfo dsStateCI = ToVKPipelineDSStateCreateInfo(mDesc.DepthStencilState);

        vk::PipelineColorBlendAttachmentState blendAttachmentState[RHI_MAX_COLOR_ATTACHMENT_COUNT] {};
        vk::PipelineColorBlendStateCreateInfo cbStateCI = ToVKPipelineCBStateCreateInfo(mDesc.BlendState, blendAttachmentState);

        vk::Format colorFormats[RHI_MAX_COLOR_ATTACHMENT_COUNT];
        vk::PipelineRenderingCreateInfo renderingCI = ToVKPipelineRenderingCreateInfo(mDesc, colorFormats);

        vk::GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.setPNext(&renderingCI);
        pipelineCI.setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT);
        pipelineCI.setStageCount(shaderStageCount);
        pipelineCI.setPStages(shaderStages);
        pipelineCI.setPViewportState(&vpStateCI);
        pipelineCI.setPRasterizationState(&rsStateCI);
        pipelineCI.setPMultisampleState(&msStateCI);
        pipelineCI.setPDepthStencilState(&dsStateCI);
        pipelineCI.setPColorBlendState(&cbStateCI);
        pipelineCI.setPDynamicState(&dyStateCI);
        pipelineCI.setLayout(device->GetPipelineLayout());

        auto deviceHandle = device->GetDevice();
        auto dynamicLoader = device->GetDynamicLoader();
        auto result = deviceHandle.createGraphicsPipelines(nullptr, 1, &pipelineCI, nullptr, &mPipeline, dynamicLoader);
        if (result != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHIGraphicsPipelineStateVK] Failed to create {}", mName);
            return false;
        }
        SetDebugName(deviceHandle, vk::ObjectType::ePipeline, (uint64_t)(VkPipeline)mPipeline, mName.c_str(), dynamicLoader);
        return true;
    }

    RHIComputePipelineStateVK::RHIComputePipelineStateVK(RHIDeviceVK *device, const RHIComputePipelineStateDesc &desc, const eastl::string &name)
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
        auto device = static_cast<RHIDeviceVK*>(mpDevice);
        device->Delete(mPipeline);

        vk::ComputePipelineCreateInfo pipelineCI {};
        pipelineCI.setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT);
        pipelineCI.stage.setStage(vk::ShaderStageFlagBits::eCompute);
        pipelineCI.stage.setModule((VkShaderModule)mDesc.CS->GetNativeHandle());
        pipelineCI.stage.setPName(mDesc.CS->GetDesc().EntryPoint.c_str());
        pipelineCI.setLayout(device->GetPipelineLayout());

        auto deviceHandle = device->GetDevice();
        auto dynamicLoader = device->GetDynamicLoader();
        auto result = deviceHandle.createComputePipelines(nullptr, 1, &pipelineCI, nullptr, &mPipeline, dynamicLoader);
        if (result != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHIComputePipelineStateVK] Failed to create {}", mName);
            return false;
        }
        SetDebugName(deviceHandle, vk::ObjectType::ePipeline, (uint64_t)(VkPipeline)mPipeline, mName.c_str(), dynamicLoader);
        return true;
    }
}