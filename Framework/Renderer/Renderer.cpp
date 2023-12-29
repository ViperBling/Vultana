#include "Renderer.hpp"

#include "Windows/GLFWindow.hpp"
#include "ShaderCompiler.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace Vultana
{
    struct Vertex
    {
        Vector3 Position;
        Vector3 Color;
    };

    RendererBase::~RendererBase()
    {
    }

    void RendererBase::Init(RendererCreateInfo &createInfo)
    {
        InitContext(createInfo);
        InitSwapchain(createInfo);
    
        InitPipelines();
        // InitCommands();
        // CreateVertexBuffer();
        // InitDescriptors();
        // InitSyncStructures();
    }

    void RendererBase::Cleanup()
    {
    }

    void RendererBase::RenderFrame()
    {
    }

    void RendererBase::InitContext(RendererCreateInfo &createInfo)
    {
        mInstance = RHIInstance::GetInstanceByRHIBackend(RHIRenderBackend::Vulkan);
        mGPU = mInstance->GetGPU(0);

        std::vector<QueueInfo> queueInfos = {
            { RHICommandQueueType::Graphics, 1 },
        };
        DeviceCreateInfo deviceCI {};
        deviceCI.QueueCreateInfoCount = queueInfos.size();
        deviceCI.QueueCreateInfos = queueInfos.data();
        mDevice = std::unique_ptr<RHIDevice>(mGPU->RequestDevice(deviceCI));
        mQueue = mDevice->GetQueue(RHICommandQueueType::Graphics, 0);
    }

    void RendererBase::InitSwapchain(RendererCreateInfo &createInfo)
    {
        static std::vector<RHIFormat> swapchainFormats = {
            RHIFormat::RGBA8_UNORM,
            RHIFormat::BGRA8_UNORM,
        };

        SurfaceCreateInfo surfaceCI {};
        surfaceCI.Window = mWndHandle->GetNativeHandle();
        mSurface = std::unique_ptr<RHISurface>(mDevice->CreateSurface(surfaceCI));

        GDebugInfoCallback("RendererBase::InitSwapchain: Surface created", "Renderer");
        for (auto format : swapchainFormats)
        {
            if (mDevice->CheckSwapchainFormatSupport(mSurface.get(), format))
            {
                mSwapchainFormat = format;
                break;
            }
        }
        assert(mSwapchainFormat != RHIFormat::Count);

        SwapchainCreateInfo swapchainCI {};
        swapchainCI.Format = mSwapchainFormat;
        swapchainCI.PresentMode = RHIPresentMode::Immediate;
        swapchainCI.Surface = mSurface.get();
        swapchainCI.Extent = { createInfo.Width, createInfo.Height };
        swapchainCI.TextureCount = mBackBufferCount;
        swapchainCI.PresentQueue = mQueue;
        mSwapchain = std::unique_ptr<RHISwapchain>(mDevice->CreateSwapchain(swapchainCI));

        for (auto i = 0; i < mBackBufferCount; i++)
        {
            mSwapchainTextures[i] = mSwapchain->GetTexture(i);

            TextureViewCreateInfo texViewCI {};
            texViewCI.Dimension = RHITextureViewDimension::TextureView2D;
            texViewCI.BaseArrayLayer = 0;
            texViewCI.BaseMipLevel = 0;
            texViewCI.ArrayLayerCount = 1;
            texViewCI.MipLevelCount = 1;
            texViewCI.Type = RHITextureViewType::ColorAttachment;
            texViewCI.TextureType = RHITextureType::Color;
            mSwapchainTextureViews[i] = std::unique_ptr<RHITextureView>(mSwapchainTextures[i]->CreateTextureView(texViewCI));
        }
        GDebugInfoCallback("RendererBase::InitSwapchain: Swapchain created", "Renderer");
    }

    void RendererBase::InitPipelines()
    {
        PipelineLayoutCreateInfo pipelineLayoutCI {};
        pipelineLayoutCI.BindGroupLayoutCount = 0;
        pipelineLayoutCI.BindGroupLayouts = nullptr;
        mPipelineLayout = std::unique_ptr<RHIPipelineLayout>(mDevice->CreatePipelineLayout(pipelineLayoutCI));

        std::vector<uint8_t> vsCode;
        CompileShader(vsCode, "./Assets/Shaders/HLSL/Triangle.hlsl", "VSMain", RHIShaderStageBits::Vertex);
        std::vector<uint8_t> psCode;
        CompileShader(psCode, "./Assets/Shaders/HLSL/Triangle.hlsl", "PSMain", RHIShaderStageBits::Pixel);

        ShaderModuleCreateInfo vsCI {};
        vsCI.Code = vsCode.data();
        vsCI.CodeSize = vsCode.size();
        mVertexShader = std::unique_ptr<RHIShaderModule>(mDevice->CreateShaderModule(vsCI));
        ShaderModuleCreateInfo psCI {};
        psCI.Code = psCode.data();
        psCI.CodeSize = psCode.size();
        mFragmentShader = std::unique_ptr<RHIShaderModule>(mDevice->CreateShaderModule(psCI));

        std::array<VertexAttribute, 2> vertexAttributes {};
        vertexAttributes[0].Format = RHIVertexFormat::FLOAT_32X3;
        vertexAttributes[0].Offset = 0;
        vertexAttributes[0].SemanticName = "POSITION";
        vertexAttributes[0].SemanticIndex = 0;
        vertexAttributes[0].Format = RHIVertexFormat::FLOAT_32X3;
        vertexAttributes[0].Offset = offsetof(Vertex, Color);
        vertexAttributes[0].SemanticName = "COLOR";
        vertexAttributes[0].SemanticIndex = 0;

        VertexBufferLayout vertexBufferLayout {};
        vertexBufferLayout.Attributes = vertexAttributes.data();
        vertexBufferLayout.AttributeCount = vertexAttributes.size();
        vertexBufferLayout.Stride = sizeof(Vertex);
        vertexBufferLayout.StepMode = RHIVertexStepMode::PerVertex;

        std::array<ColorTargetState, 1> colorTargetStates {};
        colorTargetStates[0].Format = mSwapchainFormat;
        colorTargetStates[0].WriteFlags = RHIColorWriteBits::Red | RHIColorWriteBits::Green | RHIColorWriteBits::Blue | RHIColorWriteBits::Alpha;

        GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.PipelineLayout = mPipelineLayout.get();
        pipelineCI.VertexShader = mVertexShader.get();
        pipelineCI.FragmentShader = mFragmentShader.get();
        pipelineCI.VertexState.BufferLayoutCount = 1;
        pipelineCI.VertexState.BufferLayouts = &vertexBufferLayout;
        pipelineCI.FragState.ColorTargetCount = colorTargetStates.size();
        pipelineCI.FragState.ColorTargets = colorTargetStates.data();
        pipelineCI.PrimitiveState.bDepthClip = false;
        pipelineCI.PrimitiveState.FrontFace = RHIFrontFace::CounterClockwise;
        pipelineCI.PrimitiveState.CullMode = RHICullMode::None;
        pipelineCI.PrimitiveState.TopologyType = RHIPrimitiveTopologyType::Triangle;
        pipelineCI.PrimitiveState.IndexFormat = RHIIndexFormat::UINT_16;
        pipelineCI.DepthStencilState.bDepthEnable = false;
        pipelineCI.DepthStencilState.bStencilEnable = false;
        pipelineCI.MultiSampleState.SampleCount = 1;
        mGraphicsPipeline = std::unique_ptr<RHIGraphicsPipeline>(mDevice->CreateGraphicsPipeline(pipelineCI));

        GDebugInfoCallback("RendererBase::InitPipelines: Pipeline created", "Renderer");
    }

    void RendererBase::CreateVertexBuffer()
    {
        std::vector<Vertex> vertices = {
            { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        };
        
        BufferCreateInfo bufferCI {};
        bufferCI.Size = sizeof(Vertex) * vertices.size();
        bufferCI.Usage = RHIBufferUsageBits::Vertex | RHIBufferUsageBits::MapWrite | RHIBufferUsageBits::CopySrc;
        mVertexBuffer = std::unique_ptr<RHIBuffer>(mDevice->CreateBuffer(bufferCI));
        if (mVertexBuffer)
        {
            auto* data = mVertexBuffer->Map(RHIMapMode::Write, 0, bufferCI.Size);
            memcpy(data, vertices.data(), bufferCI.Size);
            mVertexBuffer->Unmap();
        }
        BufferViewCreateInfo bufferViewCI {};
        bufferViewCI.Type = RHIBufferViewType::Vertex;
        bufferViewCI.Size = sizeof(Vertex) * vertices.size();
        bufferViewCI.Offset = 0;
        bufferViewCI.Vertex.Stride = sizeof(Vertex);
        mVertexBufferView = std::unique_ptr<RHIBufferView>(mVertexBuffer->CreateBufferView(bufferViewCI));
    }

    void RendererBase::InitSyncStructures()
    {
        mFence = std::unique_ptr<RHIFence>(mDevice->CreateFence());
    }

    void RendererBase::InitCommands()
    {
        mCommandBuffer = std::unique_ptr<RHICommandBuffer>(mDevice->CreateCommandBuffer());
    }

    void RendererBase::RecordCommandBuffer()
    {
    }

    void RendererBase::SubmitCommandBuffer()
    {
        mFence->Reset();
        mQueue->Submit(mCommandBuffer.get(), mFence.get());
        mSwapchain->Present();
        mFence->Wait();
    }

    void RendererBase::CompileShader(std::vector<uint8_t> &byteCode, const std::string &fileName, const std::string &entryPoint, RHIShaderStageBits shaderStage, std::vector<std::string> includePath)
    {
        std::string shaderSrc = FileUtils::ReadTextFile(fileName);

        ShaderCompileInput input {};
        input.Source = shaderSrc;
        input.EntryPoint = entryPoint;
        input.Stage = shaderStage;
        ShaderCompileOptions option;
        if (!includePath.empty()) { option.IncludePaths.insert(option.IncludePaths.end(), includePath.begin(), includePath.end()); }
        option.ByteCodeType = ShaderByteCodeType::SPIRV;
        option.bWithDebugInfo = true;

        auto future = ShaderCompiler::Get().Compile(input, option);

        future.wait();
        auto result = future.get();
        if (!result.bSuccess)
        {
            GDebugInfoCallback(result.ErrorMsg, "ShaderCompiler");
        }
        assert(result.bSuccess);
        byteCode = result.ByteCode;
    }
} // namespace Vultana::Renderer
