#include "RHIPipelineStateVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIShaderVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIGraphicsPipelineStateVK::RHIGraphicsPipelineStateVK(RHIDeviceVK *device, const RHIGraphicsPipelineStateDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
        m_Type = ERHIPipelineType::Graphics;
    }

    RHIGraphicsPipelineStateVK::~RHIGraphicsPipelineStateVK()
    {
        ((RHIDeviceVK*)m_pDevice)->Delete(m_Pipeline);
    }

    bool RHIGraphicsPipelineStateVK::Create()
    {
        auto device = static_cast<RHIDeviceVK*>(m_pDevice);
        device->Delete(m_Pipeline);

        vk::PipelineShaderStageCreateInfo shaderStages[2];
        shaderStages[0].setStage(vk::ShaderStageFlagBits::eVertex);
        shaderStages[0].setModule((VkShaderModule)m_Desc.VS->GetNativeHandle());
        shaderStages[0].setPName(m_Desc.VS->GetDesc().EntryPoint.c_str());
        if (m_Desc.PS)
        {
            shaderStages[1].setStage(vk::ShaderStageFlagBits::eFragment);
            shaderStages[1].setModule((VkShaderModule)m_Desc.PS->GetNativeHandle());
            shaderStages[1].setPName(m_Desc.PS->GetDesc().EntryPoint.c_str());
        }

        vk::PipelineVertexInputStateCreateInfo viStateCI {};
        vk::PipelineInputAssemblyStateCreateInfo iaStateCI {};
        iaStateCI.topology = ToVKPrimitiveTopology(m_Desc.PrimitiveType);

        vk::PipelineMultisampleStateCreateInfo m_sStateCI {};
        m_sStateCI.rasterizationSamples = vk::SampleCountFlagBits::e1;

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

        vk::PipelineRasterizationStateCreateInfo rsStateCI = ToVKPipelineRSStateCreateInfo(m_Desc.RasterizerState);
        vk::PipelineDepthStencilStateCreateInfo dsStateCI = ToVKPipelineDSStateCreateInfo(m_Desc.DepthStencilState);

        vk::PipelineColorBlendAttachmentState blendAttachmentState[RHI_MAX_COLOR_ATTACHMENT_COUNT] {};
        vk::PipelineColorBlendStateCreateInfo cbStateCI = ToVKPipelineCBStateCreateInfo(m_Desc.BlendState, blendAttachmentState);

        vk::Format colorFormats[RHI_MAX_COLOR_ATTACHMENT_COUNT];
        vk::PipelineRenderingCreateInfo renderingCI = ToVKPipelineRenderingCreateInfo(m_Desc, colorFormats);

        vk::GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.setPNext(&renderingCI);
        pipelineCI.setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT);
        pipelineCI.setStageCount(m_Desc.PS ? 2 : 1);
        pipelineCI.setPStages(shaderStages);
        pipelineCI.setPVertexInputState(&viStateCI);
        pipelineCI.setPInputAssemblyState(&iaStateCI);
        pipelineCI.setPViewportState(&vpStateCI);
        pipelineCI.setPRasterizationState(&rsStateCI);
        pipelineCI.setPMultisampleState(&m_sStateCI);
        pipelineCI.setPDepthStencilState(&dsStateCI);
        pipelineCI.setPColorBlendState(&cbStateCI);
        pipelineCI.setPDynamicState(&dyStateCI);
        pipelineCI.setLayout(device->GetPipelineLayout());

        auto deviceHandle = device->GetDevice();
        auto dynamicLoader = device->GetDynamicLoader();
        auto result = deviceHandle.createGraphicsPipelines(nullptr, 1, &pipelineCI, nullptr, &m_Pipeline, dynamicLoader);
        if (result != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHIGraphicsPipelineStateVK] Failed to create {}", m_Name);
            return false;
        }
        SetDebugName(deviceHandle, vk::ObjectType::ePipeline, (uint64_t)(VkPipeline)m_Pipeline, m_Name.c_str(), dynamicLoader);
        return true;
    }

    RHIMeshShadingPipelineStateVK::RHIMeshShadingPipelineStateVK(RHIDeviceVK *device, const RHIMeshShadingPipelineStateDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
        m_Type = ERHIPipelineType::MeshShading;
    }
    
    RHIMeshShadingPipelineStateVK::~RHIMeshShadingPipelineStateVK()
    {
        ((RHIDeviceVK*)m_pDevice)->Delete(m_Pipeline);
    }

    bool RHIMeshShadingPipelineStateVK::Create()
    {
        auto device = static_cast<RHIDeviceVK*>(m_pDevice);
        device->Delete(m_Pipeline);

        uint32_t shaderStageCount = 1;
        vk::PipelineShaderStageCreateInfo shaderStages[3];
        shaderStages[0].setStage(vk::ShaderStageFlagBits::eMeshEXT);
        shaderStages[0].setModule((VkShaderModule)m_Desc.MS->GetNativeHandle());
        shaderStages[0].setPName(m_Desc.MS->GetDesc().EntryPoint.c_str());

        if (m_Desc.AS)
        {
            shaderStages[shaderStageCount].setStage(vk::ShaderStageFlagBits::eTaskEXT);
            shaderStages[shaderStageCount].setModule((VkShaderModule)m_Desc.AS->GetNativeHandle());
            shaderStages[shaderStageCount].setPName(m_Desc.AS->GetDesc().EntryPoint.c_str());
            shaderStageCount++;
        }
        if (m_Desc.PS)
        {
            shaderStages[shaderStageCount].setStage(vk::ShaderStageFlagBits::eFragment);
            shaderStages[shaderStageCount].setModule((VkShaderModule)m_Desc.PS->GetNativeHandle());
            shaderStages[shaderStageCount].setPName(m_Desc.PS->GetDesc().EntryPoint.c_str());
            shaderStageCount++;
        }

        vk::PipelineMultisampleStateCreateInfo m_sStateCI {};
        m_sStateCI.rasterizationSamples = vk::SampleCountFlagBits::e1;

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

        vk::PipelineRasterizationStateCreateInfo rsStateCI = ToVKPipelineRSStateCreateInfo(m_Desc.RasterizerState);
        vk::PipelineDepthStencilStateCreateInfo dsStateCI = ToVKPipelineDSStateCreateInfo(m_Desc.DepthStencilState);

        vk::PipelineColorBlendAttachmentState blendAttachmentState[RHI_MAX_COLOR_ATTACHMENT_COUNT] {};
        vk::PipelineColorBlendStateCreateInfo cbStateCI = ToVKPipelineCBStateCreateInfo(m_Desc.BlendState, blendAttachmentState);

        vk::Format colorFormats[RHI_MAX_COLOR_ATTACHMENT_COUNT];
        vk::PipelineRenderingCreateInfo renderingCI = ToVKPipelineRenderingCreateInfo(m_Desc, colorFormats);

        vk::GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.setPNext(&renderingCI);
        pipelineCI.setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT);
        pipelineCI.setStageCount(shaderStageCount);
        pipelineCI.setPStages(shaderStages);
        pipelineCI.setPViewportState(&vpStateCI);
        pipelineCI.setPRasterizationState(&rsStateCI);
        pipelineCI.setPMultisampleState(&m_sStateCI);
        pipelineCI.setPDepthStencilState(&dsStateCI);
        pipelineCI.setPColorBlendState(&cbStateCI);
        pipelineCI.setPDynamicState(&dyStateCI);
        pipelineCI.setLayout(device->GetPipelineLayout());

        auto deviceHandle = device->GetDevice();
        auto dynamicLoader = device->GetDynamicLoader();
        auto result = deviceHandle.createGraphicsPipelines(nullptr, 1, &pipelineCI, nullptr, &m_Pipeline, dynamicLoader);
        if (result != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHIGraphicsPipelineStateVK] Failed to create {}", m_Name);
            return false;
        }
        SetDebugName(deviceHandle, vk::ObjectType::ePipeline, (uint64_t)(VkPipeline)m_Pipeline, m_Name.c_str(), dynamicLoader);
        return true;
    }

    RHIComputePipelineStateVK::RHIComputePipelineStateVK(RHIDeviceVK *device, const RHIComputePipelineStateDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
        m_Type = ERHIPipelineType::Compute;
    }

    RHIComputePipelineStateVK::~RHIComputePipelineStateVK()
    {
        ((RHIDeviceVK*)m_pDevice)->Delete(m_Pipeline);
    }

    bool RHIComputePipelineStateVK::Create()
    {
        auto device = static_cast<RHIDeviceVK*>(m_pDevice);
        device->Delete(m_Pipeline);

        vk::ComputePipelineCreateInfo pipelineCI {};
        pipelineCI.setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT);
        pipelineCI.stage.setStage(vk::ShaderStageFlagBits::eCompute);
        pipelineCI.stage.setModule((VkShaderModule)m_Desc.CS->GetNativeHandle());
        pipelineCI.stage.setPName(m_Desc.CS->GetDesc().EntryPoint.c_str());
        pipelineCI.setLayout(device->GetPipelineLayout());

        auto deviceHandle = device->GetDevice();
        auto dynamicLoader = device->GetDynamicLoader();
        auto result = deviceHandle.createComputePipelines(nullptr, 1, &pipelineCI, nullptr, &m_Pipeline, dynamicLoader);
        if (result != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHIComputePipelineStateVK] Failed to create {}", m_Name);
            return false;
        }
        SetDebugName(deviceHandle, vk::ObjectType::ePipeline, (uint64_t)(VkPipeline)m_Pipeline, m_Name.c_str(), dynamicLoader);
        return true;
    }
}