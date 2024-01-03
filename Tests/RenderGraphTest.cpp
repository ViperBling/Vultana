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

    }

    void Execute(RHI::RHIComputePassCommandList& cmdList) override
    {

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

struct RenderGraphTest : public ::testing::Test
{
    void SetUp() override
    {

    }
    void TearDown() override {}
};

TEST_F(RenderGraphTest, BasicTest)
{
    
}