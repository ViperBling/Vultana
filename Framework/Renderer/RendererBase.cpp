#include "RendererBase.hpp"
#include "PipelineStateCache.hpp"
#include "ShaderCompiler.hpp"
#include "ShaderCache.hpp"
#include "RHI/RHI.hpp"
#include "Core/VultanaEngine.hpp"
#include "Utilities/Log.hpp"
#include "Windows/GLFWindow.hpp"

// For Test
#include "RHI/RHIBuffer.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

namespace Renderer
{
    // For Test
    struct Vertex
    {
        Math::Vector3 Position;
        Math::Vector3 Color;
    };

    RendererBase::RendererBase()
    {
        mpShaderCache = std::make_unique<ShaderCache>(this);
        mpShaderCompiler = std::make_unique<ShaderCompiler>(this);
        mpPipelineStateCache = std::make_unique<PipelineStateCache>(this);
        
        auto onResizeCallback = std::bind(&RendererBase::OnWindowResize, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        Core::VultanaEngine::GetEngineInstance()->GetWindowHandle()->OnResize(onResizeCallback);
    }

    RendererBase::~RendererBase()
    {
        WaitGPU();
    }

    bool RendererBase::CreateDevice(RHI::ERHIRenderBackend backend, void *windowHandle, uint32_t width, uint32_t height)
    {
        mDisplayWidth = width;
        mDisplayHeight = height;
        mRenderWidth = width;
        mRenderHeight = height;

        RHI::RHIDeviceDesc deviceDesc {};
        deviceDesc.RenderBackend = backend;
        mpDevice.reset(RHI::CreateRHIDevice(deviceDesc));
        if (mpDevice == nullptr)
        {
            VTNA_LOG_ERROR("[Renderer::CreateDevice] failed to create the RHI device.");
            return false;
        }

        RHI::RHISwapchainDesc swapchainDesc {};
        swapchainDesc.Width = width;
        swapchainDesc.Height = height;
        swapchainDesc.WindowHandle = windowHandle;
        mpSwapchain.reset(mpDevice->CreateSwapchain(swapchainDesc, "RendererBase::Swapchain"));
        
        mpFrameFence.reset(mpDevice->CreateFence("RendererBase::FrameFence"));
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            std::string name = fmt::format("RendererBase::CommonCmdList{}", i).c_str();
            mpCmdList[i].reset(mpDevice->CreateCommandList(RHI::ERHICommandQueueType::Graphics, name));
        }

        mpAsyncComputeFence.reset(mpDevice->CreateFence("RendererBase::AsyncComputeFence"));
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            std::string name = fmt::format("RendererBase::AsyncComputeCmdList{}", i).c_str();
            mpAsyncComputeCmdList[i].reset(mpDevice->CreateCommandList(RHI::ERHICommandQueueType::Compute, name));
        }

        mpUploadFence.reset(mpDevice->CreateFence("RendererBase::UploadFence"));
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            std::string name = fmt::format("RendererBase::UploadCmdList{}", i).c_str();
            mpUploadCmdList[i].reset(mpDevice->CreateCommandList(RHI::ERHICommandQueueType::Copy, name));
            mpStagingBufferAllocators[i] = std::make_unique<StagingBufferAllocator>(this);
        }

        CreateCommonResources();
        
        return true;
    }

    void RendererBase::RenderFrame()
    {
        BeginFrame();
        UploadResource();
        Render();
        EndFrame();
    }

    void RendererBase::WaitGPU()
    {
        if (mpFrameFence)
        {
            mpFrameFence->Wait(mCurrentFrameFenceValue);
        }
    }

    RHI::RHIShader *RendererBase::GetShader(const std::string &file, const std::string &entryPoint, RHI::ERHIShaderType type, const std::vector<std::string> &defines, RHI::ERHIShaderCompileFlags flags)
    {
        return mpShaderCache->GetShader(file, entryPoint, type, defines, flags);
    }

    RHI::RHIPipelineState *RendererBase::GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc &desc, const std::string &name)
    {
        return mpPipelineStateCache->GetPipelineState(desc, name);
    }

    RHI::RHIPipelineState *RendererBase::GetPipelineState(const RHI::RHIComputePipelineStateDesc &desc, const std::string &name)
    {
        return mpPipelineStateCache->GetPipelineState(desc, name);
    }

    void RendererBase::ReloadShaders()
    {
        mpShaderCache->ReloadShaders();
    }

    Texture2D *RendererBase::CreateTexture2D(const std::string &file, bool srgb)
    {
        return nullptr;
    }

    Texture2D *RendererBase::CreateTexture2D(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags, const std::string &name)
    {
        Texture2D* texture = new Texture2D(name);
        if (!texture->Create(width, height, levels, format, flags))
        {
            delete texture;
            return nullptr;
        }
        return texture;
    }

    IndexBuffer *RendererBase::CreateIndexBuffer(const void *data, uint32_t stride, uint32_t indexCount, const std::string &name, RHI::ERHIMemoryType memoryType)
    {
        std::vector<uint16_t> u16IndexBuffer;
        if (stride == 1)
        {
            u16IndexBuffer.resize(indexCount);
            for (uint32_t i = 0; i < indexCount; i++)
            {
                u16IndexBuffer[i] = static_cast<const char*>(data)[i];
            }
            stride = 2;
            data = u16IndexBuffer.data();
        }

        IndexBuffer* idBuffer = new IndexBuffer(name);
        if (!idBuffer->Create(stride, indexCount, memoryType))
        {
            delete idBuffer;
            return nullptr;
        }
        if (data)
        {
            UploadBuffer(idBuffer->GetBuffer(), data, 0, stride * indexCount);
        }

        return idBuffer;
    }

    StructuredBuffer *RendererBase::CreateStructuredBuffer(const void *data, uint32_t stride, uint32_t elementCount, const std::string &name, RHI::ERHIMemoryType memoryType, bool isUAV)
    {
        StructuredBuffer* pBuffer = new StructuredBuffer(name);
        if (!pBuffer->Create(stride, elementCount, memoryType, isUAV))
        {
            delete pBuffer;
            return nullptr;
        }
        if (data)
        {
            UploadBuffer(pBuffer->GetBuffer(), data, 0, stride * elementCount);
        }
        return pBuffer;
    }

    void RendererBase::UploadBuffer(RHI::RHIBuffer *pBuffer, const void *pData, uint32_t offset, uint32_t dataSize)
    {
        uint32_t frameIndex = mpDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        StagingBufferAllocator* pAllocator = mpStagingBufferAllocators[frameIndex].get();

        StagingBuffer stagingBuffer = pAllocator->Allocate(dataSize);

        char* dstData = (char*)stagingBuffer.Buffer->GetCPUAddress() + stagingBuffer.Offset;
        memcpy(dstData, pData, dataSize);

        BufferUpload upload;
        upload.Buffer = pBuffer;
        upload.Offset = offset;
        upload.SBForUpload = stagingBuffer;
        mPendingBufferUpload.push_back(upload);
    }

    void RendererBase::CreateCommonResources()
    {
        RHI::RHISamplerDesc samplerDesc {};
        mpPointSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::PointSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MagFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MipFilter = RHI::ERHIFilter::Linear;
        mpLinearSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::LinearSampler"));

        RHI::RHITexture* pBackBuffer = mpSwapchain->GetBackBuffer();
        uint32_t width = pBackBuffer->GetDesc().Width;
        uint32_t height = pBackBuffer->GetDesc().Height;

        mpTestRT.reset(CreateTexture2D(width, height, 1, RHI::ERHIFormat::RGBA16F, RHI::RHITextureUsageRenderTarget, "PresentRT"));
        mpTestDepthRT.reset(CreateTexture2D(width, height, 1, RHI::ERHIFormat::D32F, RHI::RHITextureUsageDepthStencil, "PresentDepthRT"));

        // For Test
        std::vector<Vertex> vertices = 
        {
            { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        };
        // std::vector<Vertex> vertices = 
        // {
        //     { { -0.5f, -0.5f, 0.0f } },
        //     { {  0.5f, -0.5f, 0.0f } },
        //     { {  0.0f,  0.5f, 0.0f } },
        // };

        std::vector<uint16_t> indices = { 0, 1, 2 };

        mTestIndexBuffer.reset(CreateIndexBuffer(indices.data(), sizeof(uint16_t), static_cast<uint32_t>(indices.size()), "TriangleIndexBuffer"));
        mTestVertexBuffer.reset(CreateStructuredBuffer(vertices.data(), sizeof(Vertex), static_cast<uint32_t>(vertices.size()), "TriangleVertexBuffer"));

        RHI::RHIGraphicsPipelineStateDesc psoDesc;
        psoDesc.VS = GetShader("Triangle.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        psoDesc.PS = GetShader("Triangle.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        psoDesc.DepthStencilState.bDepthWrite = false;
        psoDesc.DepthStencilState.bDepthTest = true;
        psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::Always;
        psoDesc.RTFormats[0] = RHI::ERHIFormat::RGBA16F;
        psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

        mTestPSO = GetPipelineState(psoDesc, "TrianglePSO");

        RHI::RHIGraphicsPipelineStateDesc copyPSODesc;
        copyPSODesc.VS = GetShader("Copy.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        copyPSODesc.PS = GetShader("Copy.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        copyPSODesc.DepthStencilState.bDepthWrite = false;
        copyPSODesc.RTFormats[0] = mpSwapchain->GetDesc()->ColorFormat;
        copyPSODesc.DepthStencilFormat = RHI::ERHIFormat::D32F;
        mpCopyColorPSO = GetPipelineState(copyPSODesc, "CopyColorPSO");

        copyPSODesc.PS = GetShader("Copy.hlsl", "PSMain", RHI::ERHIShaderType::PS, { "OUTPUT_DEPTH=1" });
        copyPSODesc.DepthStencilState.bDepthWrite = true;
        copyPSODesc.DepthStencilState.bDepthTest = true;
        copyPSODesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::Always;
        mpCopyColorDepthPSO = GetPipelineState(copyPSODesc, "CopyDepthPSO");

        RHI::RHIComputePipelineStateDesc computePSODesc;
        computePSODesc.CS = GetShader("Copy.hlsl", "CSCopyDepth", RHI::ERHIShaderType::CS);
        mpCopyDepthPSO = GetPipelineState(computePSODesc, "CopyDepthComputePSO");
    }

    void RendererBase::OnWindowResize(Window::GLFWindow &wndHandle, uint32_t width, uint32_t height)
    {
        WaitGPU();
        
        mpSwapchain->Resize(width, height);
        mDisplayWidth = width;
        mDisplayHeight = height;
        mRenderWidth = mDisplayWidth;
        mRenderHeight = mDisplayHeight;
    }

    void RendererBase::BeginFrame()
    {
        uint32_t frameIndex = mpDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        mpFrameFence->Wait(mFrameFenceValue[frameIndex]);

        mpDevice->BeginFrame();

        RHI::RHICommandList* pCmdList = mpCmdList[frameIndex].get();
        RHI::RHICommandList* pComputeCmdList = mpAsyncComputeCmdList[frameIndex].get();

        pCmdList->ResetAllocator();
        pCmdList->Begin();

        pComputeCmdList->ResetAllocator();
        pComputeCmdList->Begin();
    }

    void RendererBase::UploadResource()
    {
        if (mPendingBufferUpload.empty()) return;

        uint32_t frameIndex = mpDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHICommandList* pUploadeCmdList = mpUploadCmdList[frameIndex].get();
        pUploadeCmdList->ResetAllocator();
        pUploadeCmdList->Begin();

        {
            GPU_EVENT_DEBUG(pUploadeCmdList, "RendererBase::UploadResource");
            
            for (size_t i = 0; i < mPendingBufferUpload.size(); i++)
            {
                const BufferUpload& upload = mPendingBufferUpload[i];
                pUploadeCmdList->CopyBuffer(upload.SBForUpload.Buffer, upload.Buffer, upload.SBForUpload.Offset, upload.Offset, upload.SBForUpload.Size);
            }
        }

        pUploadeCmdList->End();
        pUploadeCmdList->Signal(mpUploadFence.get(), ++mCurrentUploadFenceValue);
        pUploadeCmdList->Submit();

        RHI::RHICommandList* pCmdList = mpCmdList[frameIndex].get();
        pCmdList->Wait(mpUploadFence.get(), mCurrentUploadFenceValue);

        mPendingBufferUpload.clear();
    }

    void RendererBase::Render()
    {
        uint32_t frameIndex = mpDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHICommandList* pCmdList = mpCmdList[frameIndex].get();
        RHI::RHICommandList* pComputeCmdList = mpAsyncComputeCmdList[frameIndex].get();

        // pCmdList->TextureBarrier(mpTestRT->GetTexture(), 0, RHI::RHIAccessMaskSRV, RHI::RHIAccessRTV);
        {
            RHI::RHIRenderPassDesc renderPassDesc {};
            renderPassDesc.Color[0].Texture = mpTestRT->GetTexture();
            renderPassDesc.Color[0].LoadOp = RHI::ERHIRenderPassLoadOp::DontCare;
            renderPassDesc.Color[0].ClearColor[0] = 0.3f;
            renderPassDesc.Color[0].ClearColor[1] = 0.3f;
            renderPassDesc.Color[0].ClearColor[2] = 0.3f;
            renderPassDesc.Color[0].ClearColor[3] = 1.0f;
            renderPassDesc.Depth.Texture = mpTestDepthRT->GetTexture();
            renderPassDesc.Depth.DepthLoadOp = RHI::ERHIRenderPassLoadOp::Clear;
            renderPassDesc.Depth.StencilLoadOp = RHI::ERHIRenderPassLoadOp::Clear;
            pCmdList->BeginRenderPass(renderPassDesc);

            pCmdList->SetViewport(0, 0, mpSwapchain->GetDesc()->Width, mpSwapchain->GetDesc()->Height);
            pCmdList->SetPipelineState(mTestPSO);

            pCmdList->SetIndexBuffer(mTestIndexBuffer->GetBuffer(), 0, mTestIndexBuffer->GetFormat());
            uint32_t vertexBuffer = mTestVertexBuffer->GetSRV()->GetHeapIndex();
            pCmdList->SetGraphicsConstants(0, &vertexBuffer, sizeof(vertexBuffer));

            pCmdList->DrawIndexed(mTestIndexBuffer->GetIndexCount());

            pCmdList->EndRenderPass();
        }
        // pCmdList->TextureBarrier(mpTestRT->GetTexture(), 0, RHI::RHIAccessRTV, RHI::RHIAccessMaskSRV | RHI::RHIAccessMaskPS);

        RenderBackBufferPass(pCmdList);
    }

    void RendererBase::EndFrame()
    {
        uint32_t frameIndex = mpDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHICommandList* pCmdList = mpCmdList[frameIndex].get();
        RHI::RHICommandList* pComputeCmdList = mpAsyncComputeCmdList[frameIndex].get();

        pComputeCmdList->End();
        pCmdList->End();

        mFrameFenceValue[frameIndex] = ++mCurrentFrameFenceValue;

        pCmdList->Present(mpSwapchain.get());
        pCmdList->Signal(mpFrameFence.get(), mCurrentFrameFenceValue);
        pCmdList->Submit();

        mpDevice->EndFrame();
    }

    void RendererBase::RenderBackBufferPass(RHI::RHICommandList *pCmdList)
    {
        GPU_EVENT_DEBUG(pCmdList, "RenderBackBufferPass");

        mpSwapchain->AcquireNextBackBuffer();
        pCmdList->TextureBarrier(mpSwapchain->GetBackBuffer(), 0, RHI::RHIAccessPresent, RHI::RHIAccessRTV);

        {
            RHI::RHIRenderPassDesc renderPassDesc {};
            renderPassDesc.Color[0].Texture = mpSwapchain->GetBackBuffer();
            renderPassDesc.Color[0].LoadOp = RHI::ERHIRenderPassLoadOp::DontCare;
            renderPassDesc.Depth.Texture = mpTestDepthRT->GetTexture();
            renderPassDesc.Depth.DepthLoadOp = RHI::ERHIRenderPassLoadOp::Load;
            renderPassDesc.Depth.StencilLoadOp = RHI::ERHIRenderPassLoadOp::DontCare;
            renderPassDesc.Depth.DepthStoreOp = RHI::ERHIRenderPassStoreOp::DontCare;
            renderPassDesc.Depth.StencilStoreOp = RHI::ERHIRenderPassStoreOp::DontCare;
            renderPassDesc.Depth.bReadOnly = true;
            pCmdList->BeginRenderPass(renderPassDesc);

            uint32_t constants[3] = 
            {
                mpTestRT->GetSRV()->GetHeapIndex(),
                mpTestDepthRT->GetSRV()->GetHeapIndex(),
                mpPointSampler->GetHeapIndex()
            };
            pCmdList->SetGraphicsConstants(0, constants, sizeof(constants));
            pCmdList->SetPipelineState(mpCopyColorPSO);
            pCmdList->Draw(3);

            pCmdList->EndRenderPass();
        }

        pCmdList->TextureBarrier(mpSwapchain->GetBackBuffer(), 0, RHI::RHIAccessRTV, RHI::RHIAccessPresent);
    }
} // namespace Vultana::Renderer
