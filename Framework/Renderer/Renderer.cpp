#include "Renderer.hpp"

#include "Windows/GLFWindow.hpp"
#include "ShaderCompiler.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace Renderer
{
    struct Vertex
    {
        Math::Vector3 Position;
        Math::Vector3 Color;
    };

    RendererBase::~RendererBase()
    {
        Cleanup();
    }

    void RendererBase::Init(RendererCreateInfo &createInfo)
    {
        InitContext(createInfo);
        InitSwapchain(createInfo);
    
        InitPipelines();
        CreateVertexBuffer();
        InitSyncStructures();
        InitCommands();

        GDebugInfoCallback("RendererBase::Init: Renderer initialized", "Renderer");
    }

    void RendererBase::Cleanup()
    {
        mQueue->Wait(mFence.get());
        mFence->Wait();
    }

    void RendererBase::RenderFrame()
    {
        RecordCommandBuffer();
        SubmitCommandBuffer();
    }

    void RendererBase::InitContext(RendererCreateInfo &createInfo)
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
        mRenderGraph = std::make_unique<RenderGraph>(*mDevice.get());
        mQueue = mDevice->GetQueue(RHI::RHICommandQueueType::Graphics, 0);
    }

    void RendererBase::InitSwapchain(RendererCreateInfo &createInfo)
    {
        static std::vector<RHI::RHIFormat> swapchainFormats = {
            RHI::RHIFormat::RGBA8_UNORM,
            RHI::RHIFormat::BGRA8_UNORM,
        };

        RHI::SurfaceCreateInfo surfaceCI {};
        surfaceCI.Window = mWndHandle->GetNativeHandle();
        mSurface = std::unique_ptr<RHI::RHISurface>(mDevice->CreateSurface(surfaceCI));

        GDebugInfoCallback("RendererBase::InitSwapchain: Surface created", "Renderer");
        for (auto format : swapchainFormats)
        {
            if (mDevice->CheckSwapchainFormatSupport(mSurface.get(), format))
            {
                mSwapchainFormat = format;
                break;
            }
        }
        assert(mSwapchainFormat != RHI::RHIFormat::Count);

        RHI::SwapchainCreateInfo swapchainCI {};
        swapchainCI.Format = mSwapchainFormat;
        swapchainCI.PresentMode = RHI::RHIPresentMode::Immediate;
        swapchainCI.Surface = mSurface.get();
        swapchainCI.Extent = { createInfo.Width, createInfo.Height };
        swapchainCI.TextureCount = mBackBufferCount;
        swapchainCI.PresentQueue = mQueue;
        mSwapchain = std::unique_ptr<RHI::RHISwapchain>(mDevice->CreateSwapchain(swapchainCI));

        mSwapchainExtent = { createInfo.Width, createInfo.Height };

        mWndHandle->OnResize([this, &swapchainCI](Window::GLFWindow& window, Math::Vector2 size)
        {
            mSwapchainExtent = size;
            RHI::SwapchainCreateInfo swapchainCI {};
            swapchainCI.Format = mSwapchainFormat;
            swapchainCI.PresentMode = RHI::RHIPresentMode::Immediate;
            swapchainCI.Surface = mSurface.get();
            swapchainCI.Extent = size;
            swapchainCI.TextureCount = mBackBufferCount;
            swapchainCI.PresentQueue = mQueue;
            mSwapchain->Resize(swapchainCI);

            CreateSwapchainImageView();
        });

        CreateSwapchainImageView();
        GDebugInfoCallback("RendererBase::InitSwapchain: Swapchain created", "Renderer");
    }

    void RendererBase::CreateSwapchainImageView()
    {
        for (auto i = 0; i < mBackBufferCount; i++)
        {
            mSwapchainTextures[i] = mSwapchain->GetTexture(i);

            RHI::TextureViewCreateInfo texViewCI {};
            texViewCI.Dimension = RHI::RHITextureViewDimension::TextureView2D;
            texViewCI.BaseArrayLayer = 0;
            texViewCI.BaseMipLevel = 0;
            texViewCI.ArrayLayerCount = 1;
            texViewCI.MipLevelCount = 1;
            texViewCI.Type = RHI::RHITextureViewType::ColorAttachment;
            texViewCI.TextureType = RHI::RHITextureType::Color;
            mSwapchainTextureViews[i] = std::unique_ptr<RHI::RHITextureView>(mSwapchainTextures[i]->CreateTextureView(texViewCI));
        }
    }

    void RendererBase::InitPipelines()
    {
        RHI::PipelineLayoutCreateInfo pipelineLayoutCI {};
        pipelineLayoutCI.BindGroupLayoutCount = 0;
        pipelineLayoutCI.BindGroupLayouts = nullptr;
        mPipelineLayout = std::unique_ptr<RHI::RHIPipelineLayout>(mDevice->CreatePipelineLayout(pipelineLayoutCI));

        std::vector<uint8_t> vsCode;
        CompileShader(vsCode, "./Assets/Shaders/HLSL/Triangle.hlsl", VS_ENTRY_POINT, RHI::RHIShaderStageBits::Vertex);
        std::vector<uint8_t> psCode;
        CompileShader(psCode, "./Assets/Shaders/HLSL/Triangle.hlsl", PS_ENTRY_POINT, RHI::RHIShaderStageBits::Pixel);

        RHI::ShaderModuleCreateInfo vsCI {};
        vsCI.Code = vsCode.data();
        vsCI.CodeSize = vsCode.size();
        mVertexShader = std::unique_ptr<RHI::RHIShaderModule>(mDevice->CreateShaderModule(vsCI));
        RHI::ShaderModuleCreateInfo psCI {};
        psCI.Code = psCode.data();
        psCI.CodeSize = psCode.size();
        mFragmentShader = std::unique_ptr<RHI::RHIShaderModule>(mDevice->CreateShaderModule(psCI));

        std::array<RHI::VertexAttribute, 2> vertexAttributes {};
        vertexAttributes[0].Format = RHI::RHIVertexFormat::FLOAT_32X3;
        vertexAttributes[0].Offset = 0;
        vertexAttributes[0].SemanticName = "POSITION";
        vertexAttributes[0].SemanticIndex = 0;
        vertexAttributes[1].Format = RHI::RHIVertexFormat::FLOAT_32X3;
        vertexAttributes[1].Offset = offsetof(Vertex, Color);
        vertexAttributes[1].SemanticName = "COLOR";
        vertexAttributes[1].SemanticIndex = 0;

        RHI::VertexBufferLayout vertexBufferLayout {};
        vertexBufferLayout.Attributes = vertexAttributes.data();
        vertexBufferLayout.AttributeCount = vertexAttributes.size();
        vertexBufferLayout.Stride = sizeof(Vertex);
        vertexBufferLayout.StepMode = RHI::RHIVertexStepMode::PerVertex;

        std::array<RHI::ColorTargetState, 1> colorTargetStates {};
        colorTargetStates[0].Format = mSwapchainFormat;
        colorTargetStates[0].WriteFlags = RHI::RHIColorWriteBits::Red | RHI::RHIColorWriteBits::Green | RHI::RHIColorWriteBits::Blue | RHI::RHIColorWriteBits::Alpha;

        RHI::GraphicsPipelineCreateInfo pipelineCI {};
        pipelineCI.PipelineLayout = mPipelineLayout.get();
        pipelineCI.VertexShader = mVertexShader.get();
        pipelineCI.FragmentShader = mFragmentShader.get();
        pipelineCI.VertexState.BufferLayoutCount = 1;
        pipelineCI.VertexState.BufferLayouts = &vertexBufferLayout;
        pipelineCI.FragState.ColorTargetCount = colorTargetStates.size();
        pipelineCI.FragState.ColorTargets = colorTargetStates.data();
        pipelineCI.PrimitiveState.bDepthClip = false;
        pipelineCI.PrimitiveState.FrontFace = RHI::RHIFrontFace::CounterClockwise;
        pipelineCI.PrimitiveState.CullMode = RHI::RHICullMode::None;
        pipelineCI.PrimitiveState.TopologyType = RHI::RHIPrimitiveTopologyType::Triangle;
        pipelineCI.PrimitiveState.IndexFormat = RHI::RHIIndexFormat::UINT_16;
        pipelineCI.DepthStencilState.bDepthEnable = false;
        pipelineCI.DepthStencilState.bStencilEnable = false;
        pipelineCI.MultiSampleState.SampleCount = 1;
        mGraphicsPipeline = std::unique_ptr<RHI::RHIGraphicsPipeline>(mDevice->CreateGraphicsPipeline(pipelineCI));

        GDebugInfoCallback("RendererBase::InitPipelines: Pipeline created", "Renderer");
    }

    void RendererBase::CreateVertexBuffer()
    {
        std::vector<Vertex> vertices = {
            { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        };
        
        RHI::BufferCreateInfo bufferCI {};
        bufferCI.Size = sizeof(Vertex) * vertices.size();
        bufferCI.Usage = RHI::RHIBufferUsageBits::Vertex | RHI::RHIBufferUsageBits::MapWrite | RHI::RHIBufferUsageBits::CopySrc;
        mVertexBuffer = std::unique_ptr<RHI::RHIBuffer>(mDevice->CreateBuffer(bufferCI));
        if (mVertexBuffer)
        {
            auto* data = mVertexBuffer->Map(RHI::RHIMapMode::Write, 0, bufferCI.Size);
            memcpy(data, vertices.data(), bufferCI.Size);
            mVertexBuffer->Unmap();
        }
        RHI::BufferViewCreateInfo bufferViewCI {};
        bufferViewCI.Type = RHI::RHIBufferViewType::Vertex;
        bufferViewCI.Size = sizeof(Vertex) * vertices.size();
        bufferViewCI.Offset = 0;
        bufferViewCI.Vertex.Stride = sizeof(Vertex);
        mVertexBufferView = std::unique_ptr<RHI::RHIBufferView>(mVertexBuffer->CreateBufferView(bufferViewCI));

        GDebugInfoCallback("RendererBase::CreateVertexBuffer: Vertex buffer created", "Renderer");
    }

    void RendererBase::InitSyncStructures()
    {
        mFence = std::unique_ptr<RHI::RHIFence>(mDevice->CreateFence());
        GDebugInfoCallback("RendererBase::InitSyncStructures: Fence created", "Renderer");
    }

    void RendererBase::InitCommands()
    {
        mCommandBuffer = std::unique_ptr<RHI::RHICommandBuffer>(mDevice->CreateCommandBuffer());
        GDebugInfoCallback("RendererBase::InitCommands: Command buffer created", "Renderer");
    }

    void RendererBase::RecordCommandBuffer()
    {
        auto backTextureIndex = mSwapchain->AcquireBackTexture();
        auto cmdList = std::unique_ptr<RHI::RHICommandList>(mCommandBuffer->Begin());
        {
            std::array<RHI::GraphicsPassColorAttachment, 1> colorAttachments {};
            colorAttachments[0].TextureView = mSwapchainTextureViews[backTextureIndex].get();
            colorAttachments[0].LoadOp = RHI::RHILoadOp::Clear;
            colorAttachments[0].StoreOp = RHI::RHIStoreOp::Store;
            colorAttachments[0].ColorClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
            colorAttachments[0].ResolveTextureView = nullptr;

            RHI::GraphicsPassBeginInfo passBI {};
            passBI.ColorAttachmentCount = colorAttachments.size();
            passBI.ColorAttachments = colorAttachments.data();
            passBI.DepthStencilAttachment = nullptr;

            cmdList->ResourceBarrier(RHI::RHIBarrier::Transition(mSwapchainTextures[backTextureIndex], RHI::RHITextureState::Present, RHI::RHITextureState::RenderTarget));
            auto graphicsCmdList = std::unique_ptr<RHI::RHIGraphicsPassCommandList>(cmdList->BeginGraphicsPass(&passBI));
            {
                graphicsCmdList->SetPipeline(mGraphicsPipeline.get());
                graphicsCmdList->SetScissor(0, 0, mSwapchainExtent.x, mSwapchainExtent.y);
                graphicsCmdList->SetViewport(0, 0, static_cast<float>(mSwapchainExtent.x), static_cast<float>(mSwapchainExtent.y), 0, 1);
                graphicsCmdList->SetPrimitiveTopology(RHI::RHIPrimitiveTopologyType::Triangle);
                graphicsCmdList->SetVertexBuffers(0, mVertexBufferView.get());
                graphicsCmdList->Draw(3, 1, 0, 0);
            }
            graphicsCmdList->EndPass();
            cmdList->ResourceBarrier(RHI::RHIBarrier::Transition(mSwapchainTextures[backTextureIndex], RHI::RHITextureState::RenderTarget, RHI::RHITextureState::Present));
        }
        cmdList->SwapchainSync(mSwapchain.get());
        cmdList->End();
    }

    void RendererBase::SubmitCommandBuffer()
    {
        mFence->Reset();
        mQueue->Submit(mCommandBuffer.get(), mFence.get());
        mSwapchain->Present();
        mFence->Wait();
    }

    void RendererBase::CompileShader(std::vector<uint8_t> &byteCode, const std::string &fileName, const std::string &entryPoint, RHI::RHIShaderStageBits shaderStage, std::vector<std::string> includePath)
    {
        std::string shaderSrc = Utility::FileUtils::ReadTextFile(fileName);

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
