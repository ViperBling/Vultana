#include <gtest/gtest.h>

#include "RHI/RHIPCH.hpp"
#include "Renderer/RenderGraph.hpp"
#include "Renderer/Pipeline.hpp"
#include "Renderer/Shader.hpp"

class RenderGraphTestCS : public Renderer::GlobalShader
{
public:
    ShaderInfo(
        "RenderGraphTestCS",
        "/Engine/Shaders/HLSL/RenderGraphTest.hlsl",
        "CSMain",
        Renderer::ShaderStage::Compute
    );

    VariantSet();
    DefaultVariantFilter
};
RegisterGlobalShader(RenderGraphTestCS);

class TestComputePass : public Renderer::RGComputePass
{
public:
    TestComputePass() : RGComputePass("Test Compute Pass") {}

protected:
    void Setup(Renderer::RGComputePassBuilder& rgBuilder) override
    {
        RHI::RHIDevice& device = rgBuilder.GetDevice();

        RenderGraphTestCS::VariantSet varSet;
        Renderer::ShaderInstance testCS = Renderer::GlobalShaderMap<RenderGraphTestCS>::Get(device).GetShaderInstance(varSet);
        Renderer::ComputePipelineShaderSet shaders = { testCS };

        Renderer::ComputePipelineStateDesc pipelineDesc;
        pipelineDesc.ShaderSet = shaders;
        mPiplineState = Renderer::PipelineCache::Get(device).GetPipeline(pipelineDesc);

        rgBuilder.SetAsyncCompute(false);

        Renderer::RGBufferDescriptor bufferDesc;
        bufferDesc.Size = sizeof(Parameters);
        bufferDesc.Usage = RHI::RHIBufferUsageBits::Uniform;
        bufferDesc.Name = "TestUniformBuffer";
        mUniformBuffer = rgBuilder.CreateBuffer("TestUniformBuffer", bufferDesc);

        Renderer::RGTextureDescriptor texDesc;
        texDesc.Dimension = RHI::RHITextureDimension::Texture2D;
        texDesc.Extent = { 1024, 1024, 0};
        texDesc.Format = RHI::RHIFormat::BGRA8_UNORM;
        texDesc.Usage = RHI::RHITextureUsageBits::StorageBinding;
        texDesc.MipLevels = 1;
        texDesc.Samples = 1;
        texDesc.InitialState = RHI::RHITextureState::Undefined;
        mOutputTexture = rgBuilder.CreateTexture("TestOutputTexture", texDesc);

        Renderer::RGBufferViewDescriptor bufferViewDesc;
        bufferViewDesc.Type = RHI::RHIBufferViewType::UniformBinding;
        bufferViewDesc.Size = sizeof(Parameters);
        bufferViewDesc.Offset = 0;
        auto* uniformBufferView = rgBuilder.CreateBufferView(mUniformBuffer, bufferViewDesc);

        Renderer::RGTextureViewDescriptor textureViewDesc;
        textureViewDesc.Type = RHI::RHITextureViewType::StorageBinding;
        textureViewDesc.Dimension = RHI::RHITextureViewDimension::TextureView2D;
        textureViewDesc.TextureType = RHI::RHITextureType::Color;
        textureViewDesc.BaseArrayLayer = 0;
        textureViewDesc.ArrayLayerCount = 1;
        textureViewDesc.BaseMipLevel = 0;
        textureViewDesc.MipLevelCount = 1;
        auto* outputTextureView = rgBuilder.CreateTextureView(mOutputTexture, textureViewDesc);

        rgBuilder.MarkAsConsumed(mOutputTexture);
        mBindGroup = rgBuilder.AllocateBindGroup(Renderer::RGBindGroupDescriptor::Create(
            mPiplineState->GetBindGroupLayout(0),
            Renderer::RGBindItem::UniformBuffer("InputBuffer", uniformBufferView),
            Renderer::RGBindItem::StorageTexture("OutputTexture", outputTextureView)
        ));
    }

    void Execute(RHI::RHIComputePassCommandList& cmdList) override
    {
        mParams.FrameCount++;
        mUniformBuffer->UploadData(mParams);

        cmdList.SetPipeline(mPiplineState->GetRHI());
        cmdList.SetBindGroup(0, mBindGroup->GetRHI());
        cmdList.Dispatch(8, 8, 1);
    }

private:
    struct Parameters
    {
        size_t FrameCount = 0;
    } mParams;

    Renderer::RGBuffer* mUniformBuffer = nullptr;
    Renderer::RGTexture* mOutputTexture = nullptr;
    Renderer::RGBindGroup* mBindGroup = nullptr;
    Renderer::ComputePipelineState* mPiplineState = nullptr;
};

struct RenderGraphTest : public testing::Test
{
    void SetUp() override
    {
        mInstance = RHI::RHIInstance::GetInstanceByRHIBackend(RHI::RHIRenderBackend::Vulkan);
        mGPU = mInstance->GetGPU(0);

        std::vector<RHI::QueueInfo> queueInfos = {
            { RHI::RHICommandQueueType::Graphics, 1 },
        };
        RHI::DeviceCreateInfo deviceCI {};
        deviceCI.QueueCreateInfoCount = queueInfos.size();
        deviceCI.QueueCreateInfos = queueInfos.data();
        mDevice = std::unique_ptr<RHI::RHIDevice>(mGPU->RequestDevice(deviceCI));
        mQueue = mDevice->GetQueue(RHI::RHICommandQueueType::Graphics, 0);
    }
    void TearDown() override {}

    RHI::RHIGPU* mGPU = nullptr;
    RHI::RHIInstance* mInstance = nullptr;
    RHI::RHIQueue* mQueue = nullptr;
    std::unique_ptr<RHI::RHIDevice> mDevice;
};

TEST_F(RenderGraphTest, BasicTest)
{
    auto mainFence = std::unique_ptr<RHI::RHIFence>(mDevice->CreateFence());

    TestComputePass testPass;

    Renderer::RenderGraph renderGraph(*mDevice);
    renderGraph.AddComputePass(&testPass);
    renderGraph.Setup();
    renderGraph.Compile();
    renderGraph.Execute(mainFence.get());
    mainFence->Wait();
}