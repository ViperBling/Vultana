#include "PipelineVK.hpp"

#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"
#include "PipelineLayoutVK.hpp"
#include "ShaderModuleVK.hpp"

namespace Vultana
{
    static const char* GetShaderEntry(vk::ShaderStageFlagBits stage)
    {
        switch (stage)
        {
        case vk::ShaderStageFlagBits::eVertex: return VS_ENTRY_POINT.c_str();
        case vk::ShaderStageFlagBits::eFragment: return PS_ENTRY_POINT.c_str();
        default: assert(false); return nullptr;
        }
    }

    static vk::StencilOpState ConvertStencilOp(const StencilFaceState& state, uint32_t readMask, uint32_t writeMask)
    {
        vk::StencilOpState result;
        result.failOp = VKEnumCast<RHIStencilOp, vk::StencilOp>(state.FailOp);
        result.passOp = VKEnumCast<RHIStencilOp, vk::StencilOp>(state.PassOp);
        result.depthFailOp = VKEnumCast<RHIStencilOp, vk::StencilOp>(state.DepthFailOp);
        result.compareOp = VKEnumCast<RHICompareOp, vk::CompareOp>(state.CompareOp);
        result.compareMask = readMask;
        result.writeMask = writeMask;
        result.reference = 0;
        return result;
    }

    static vk::PipelineDepthStencilStateCreateInfo DepthStencilStateCI(const GraphicsPipelineCreateInfo& createInfo)
    {
        const auto& dsState = createInfo.DepthStencilState;
        vk::PipelineDepthStencilStateCreateInfo dsStateCI {};
        dsStateCI.depthTestEnable = dsState.bDepthEnable;
        dsStateCI.depthWriteEnable = dsState.bDepthEnable;
        dsStateCI.stencilTestEnable = dsState.bStencilEnable;
        dsStateCI.depthCompareOp = VKEnumCast<RHICompareOp, vk::CompareOp>(dsState.DepthCompareOp);
        dsStateCI.front = ConvertStencilOp(dsState.StencilFront, dsState.StencilReadMask, dsState.StencilWriteMask);
        dsStateCI.back = ConvertStencilOp(dsState.StencilBack, dsState.StencilReadMask, dsState.StencilWriteMask);
        dsStateCI.minDepthBounds = -1.0f;
        dsStateCI.maxDepthBounds = 1.0f;
        dsStateCI.depthBoundsTestEnable = VK_FALSE;
        return dsStateCI;
    }

    static vk::PipelineInputAssemblyStateCreateInfo InputAssemblyStateCI(const GraphicsPipelineCreateInfo& createInfo)
    {
        vk::PipelineInputAssemblyStateCreateInfo iaStateCI {};
        iaStateCI.topology = VKEnumCast<RHIPrimitiveTopologyType, vk::PrimitiveTopology>(createInfo.PrimitiveState.TopologyType);
        iaStateCI.primitiveRestartEnable = VK_FALSE;
        return iaStateCI;
    }

    static vk::PipelineRasterizationStateCreateInfo RasterizationStateCI(const GraphicsPipelineCreateInfo& createInfo)
    {
        vk::PipelineRasterizationStateCreateInfo rsStateCI {};
        rsStateCI.cullMode = VKEnumCast<RHICullMode, vk::CullModeFlagBits>(createInfo.PrimitiveState.CullMode);
        rsStateCI.frontFace = createInfo.PrimitiveState.FrontFace == RHIFrontFace::Clockwise ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise;
        rsStateCI.depthBiasClamp = createInfo.DepthStencilState.DepthBiasClamp;
        rsStateCI.depthBiasConstantFactor = static_cast<float>(createInfo.DepthStencilState.DepthBias);
        rsStateCI.depthBiasSlopeFactor = createInfo.DepthStencilState.DepthBiasSlopScale;
        rsStateCI.depthBiasEnable = createInfo.DepthStencilState.DepthBias != 0;
        rsStateCI.lineWidth = 1.0;
        rsStateCI.depthClampEnable = VK_FALSE;

        return rsStateCI;
    }

    static vk::PipelineMultisampleStateCreateInfo MultisampleStateCI(const GraphicsPipelineCreateInfo& createInfo)
    {
        vk::PipelineMultisampleStateCreateInfo msStateCI {};
        msStateCI.rasterizationSamples = static_cast<vk::SampleCountFlagBits>(createInfo.MultiSampleState.SampleCount);
        msStateCI.alphaToCoverageEnable = createInfo.MultiSampleState.bAlphaToCoverage;
        msStateCI.pSampleMask = &createInfo.MultiSampleState.SampleMask;
        
        return msStateCI;
    }

    static vk::PipelineViewportStateCreateInfo ViewportStateCI(const GraphicsPipelineCreateInfo& createInfo)
    {
        vk::PipelineViewportStateCreateInfo vpStateCI {};
        vpStateCI.viewportCount = 1;
        vpStateCI.pViewports = nullptr;
        vpStateCI.scissorCount = 1;
        vpStateCI.pScissors = nullptr;
        return vpStateCI;
    }

    static vk::PipelineColorBlendStateCreateInfo ColorBlendStateCI(const GraphicsPipelineCreateInfo& createInfo, std::vector<vk::PipelineColorBlendAttachmentState>& blendStates)
    {
        blendStates.resize(createInfo.FragState.ColorTargetCount);

        vk::PipelineColorBlendStateCreateInfo cbStateCI {};
        for (uint8_t i = 0; i < createInfo.FragState.ColorTargetCount; i++)
        {
            vk::PipelineColorBlendAttachmentState& blendState = blendStates[i];
            const auto& srcState = createInfo.FragState.ColorTargets[i];

            blendState.blendEnable = true;
            blendState.colorWriteMask = static_cast<vk::ColorComponentFlags>(srcState.WriteFlags.Value());
            blendState.alphaBlendOp = VKEnumCast<RHIBlendOp, vk::BlendOp>(srcState.Blend.Alpha.Op);
            blendState.colorBlendOp = VKEnumCast<RHIBlendOp, vk::BlendOp>(srcState.Blend.Color.Op);
            blendState.srcColorBlendFactor = VKEnumCast<RHIBlendFactor, vk::BlendFactor>(srcState.Blend.Color.Src);
            blendState.dstColorBlendFactor = VKEnumCast<RHIBlendFactor, vk::BlendFactor>(srcState.Blend.Color.Dst);
            blendState.srcAlphaBlendFactor = VKEnumCast<RHIBlendFactor, vk::BlendFactor>(srcState.Blend.Alpha.Src);
            blendState.dstAlphaBlendFactor = VKEnumCast<RHIBlendFactor, vk::BlendFactor>(srcState.Blend.Alpha.Dst);
        }
        cbStateCI.setAttachments(blendStates);
        cbStateCI.logicOpEnable = VK_FALSE;
        cbStateCI.logicOp = vk::LogicOp::eClear;
        cbStateCI.blendConstants[0] = 0.0f;
        cbStateCI.blendConstants[1] = 0.0f;
        cbStateCI.blendConstants[2] = 0.0f;
        cbStateCI.blendConstants[3] = 0.0f;

        return cbStateCI;
    }

    static vk::PipelineVertexInputStateCreateInfo VertexInputStateCI(
        const GraphicsPipelineCreateInfo& createInfo,
        std::vector<vk::VertexInputAttributeDescription>& attributes,
        std::vector<vk::VertexInputBindingDescription>& bindings)
    {
        auto* vs = static_cast<ShaderModuleVK*>(createInfo.VertexShader);
        const auto& locationTable = vs->GetLocationTable();

        vk::PipelineVertexInputStateCreateInfo viStateCI {};

        bindings.resize(createInfo.VertexState.BufferLayoutCount);
        for (size_t i = 0; i < createInfo.VertexState.BufferLayoutCount; i++)
        {
            const auto& binding = createInfo.VertexState.BufferLayouts[i];
            bindings[i].binding = i;
            bindings[i].stride = binding.Stride;
            bindings[i].inputRate = binding.StepMode == RHIVertexStepMode::PerInstance ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex;

            for (size_t j = 0; j < binding.AttributeCount; j++)
            {
                vk::VertexInputAttributeDescription attribute {};
                auto inputName = std::string("in.var.") + std::string(binding.Attributes[j].SemanticName);
                auto it = locationTable.find(inputName);
                assert(it != locationTable.end());

                attribute.binding = i;
                attribute.location = it->second;
                attribute.offset = binding.Attributes[j].Offset;
                attribute.format = VKEnumCast<RHIVertexFormat, vk::Format>(binding.Attributes[j].Format);
                attributes.push_back(attribute);
            }
        }
        viStateCI.setVertexAttributeDescriptions(attributes);
        viStateCI.setVertexBindingDescriptions(bindings);

        return viStateCI;
    }

    GraphicsPipelineVK::GraphicsPipelineVK(DeviceVK &device, const GraphicsPipelineCreateInfo &createInfo)
        : RHIGraphicsPipeline(createInfo)
        , mDevice(device)
    {
        SavePipelineLayout(createInfo);
        CreateNativeGraphicsPipeline(createInfo);
    }

    GraphicsPipelineVK::~GraphicsPipelineVK()
    {
        Destroy();
    }

    void GraphicsPipelineVK::Destroy()
    {
        if (mRenderPass) 
        { 
            mDevice.GetVkDevice().destroyRenderPass(mRenderPass); 
            mRenderPass = VK_NULL_HANDLE; 
        }
        if (mPipeline)
        {
            mDevice.GetVkDevice().destroyPipeline(mPipeline);
            mPipeline = VK_NULL_HANDLE;
        }
    }

    PipelineLayoutVK* GraphicsPipelineVK::GetPipelineLayout() const
    {
        return mPipelineLayout;
    }

    void GraphicsPipelineVK::SavePipelineLayout(const GraphicsPipelineCreateInfo &createInfo)
    {
        auto * layout = dynamic_cast<PipelineLayoutVK*>(createInfo.PipelineLayout);
        assert(layout);
        mPipelineLayout = layout;
    }

    void GraphicsPipelineVK::CreateNativeGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo)
    {
        std::vector<vk::PipelineShaderStageCreateInfo> ssCIs;
        auto setStage = [&ssCIs](RHIShaderModule* module, vk::ShaderStageFlagBits stage)
        {
            if (module)
            {
                vk::PipelineShaderStageCreateInfo ssCI {};
                ssCI.stage = stage;
                ssCI.module = static_cast<ShaderModuleVK*>(module)->GetVkShaderModule();
                ssCI.pName = GetShaderEntry(stage);
                ssCIs.push_back(ssCI);
            }
        };
        setStage(createInfo.VertexShader, vk::ShaderStageFlagBits::eVertex);
        setStage(createInfo.FragmentShader, vk::ShaderStageFlagBits::eFragment);

        std::vector<vk::DynamicState> dyStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::ePrimitiveTopology};

        vk::PipelineDynamicStateCreateInfo dyStateCI {};
        dyStateCI.setDynamicStates(dyStates);

        vk::PipelineMultisampleStateCreateInfo msStateCI = MultisampleStateCI(createInfo);
        vk::PipelineViewportStateCreateInfo vpStateCI = ViewportStateCI(createInfo);
        vk::PipelineRasterizationStateCreateInfo rsStateCI = RasterizationStateCI(createInfo);
        vk::PipelineInputAssemblyStateCreateInfo iaStateCI = InputAssemblyStateCI(createInfo);
        vk::PipelineDepthStencilStateCreateInfo dsStateCI = DepthStencilStateCI(createInfo);

        std::vector<vk::PipelineColorBlendAttachmentState> blendStates;
        vk::PipelineColorBlendStateCreateInfo cbStateCI = ColorBlendStateCI(createInfo, blendStates);

        std::vector<vk::VertexInputAttributeDescription> attributes;
        std::vector<vk::VertexInputBindingDescription> bindings;
        vk::PipelineVertexInputStateCreateInfo viStateCI = VertexInputStateCI(createInfo, attributes, bindings);

        std::vector<vk::Format> pixelFormat(createInfo.FragState.ColorTargetCount);
        for (size_t i = 0; i < createInfo.FragState.ColorTargetCount; i++)
        {
            auto format = createInfo.FragState.ColorTargets[i].Format;
            pixelFormat[i] = VKEnumCast<RHIFormat, vk::Format>(format);
        }

        vk::PipelineRenderingCreateInfo renderingCI {};
        renderingCI.setColorAttachmentCount(createInfo.FragState.ColorTargetCount);
        renderingCI.setColorAttachmentFormats(pixelFormat);
        renderingCI.setDepthAttachmentFormat(createInfo.DepthStencilState.bDepthEnable ? VKEnumCast<RHIFormat, vk::Format>(createInfo.DepthStencilState.Format) : vk::Format::eUndefined);
        renderingCI.setStencilAttachmentFormat(createInfo.DepthStencilState.bStencilEnable ? VKEnumCast<RHIFormat, vk::Format>(createInfo.DepthStencilState.Format) : vk::Format::eUndefined);
        renderingCI.setViewMask(0);

        vk::GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.setStages(ssCIs);
        pipelineCI.setLayout(static_cast<const PipelineLayoutVK*>(createInfo.PipelineLayout)->GetVkPipelineLayout());
        pipelineCI.setPDynamicState(&dyStateCI);
        pipelineCI.setPMultisampleState(&msStateCI);
        pipelineCI.setPDepthStencilState(&dsStateCI);
        pipelineCI.setPInputAssemblyState(&iaStateCI);
        pipelineCI.setPViewportState(&vpStateCI);
        pipelineCI.setPRasterizationState(&rsStateCI);
        pipelineCI.setPColorBlendState(&cbStateCI);
        pipelineCI.setPVertexInputState(&viStateCI);
        pipelineCI.setPNext(&renderingCI);

        mPipeline = mDevice.GetVkDevice().createGraphicsPipeline(nullptr, pipelineCI).value;

        if (!createInfo.Name.empty())
        {
            mDevice.SetObjectName(vk::ObjectType::ePipeline, reinterpret_cast<uint64_t>(VkPipeline(mPipeline)), createInfo.Name.c_str());
        }
    }

    void GraphicsPipelineVK::CreateNativeRenderPass(const GraphicsPipelineCreateInfo &createInfo)
    {
    }
}