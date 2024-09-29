#include "RendererBase.hpp"
#include "PipelineStateCache.hpp"
#include "ShaderCompiler.hpp"
#include "ShaderCache.hpp"
#include "RHI/RHI.hpp"
#include "Core/VultanaEngine.hpp"
#include "Core/VultanaGUI.hpp"
#include "Utilities/Log.hpp"
#include "Window/GLFWindow.hpp"

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
        float3 Position;
        float3 Color;
    };

    RendererBase::RendererBase()
    {
        mpShaderCache = std::make_unique<ShaderCache>(this);
        mpShaderCompiler = std::make_unique<ShaderCompiler>(this);
        mpPipelineStateCache = std::make_unique<PipelineStateCache>(this);
        
        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.connect(&RendererBase::OnWindowResize, this);
    }

    RendererBase::~RendererBase()
    {
        WaitGPU();

        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.disconnect(this);
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

    RenderResources::Texture2D *RendererBase::CreateTexture2D(const std::string &file, bool srgb)
    {
        return nullptr;
    }

    RenderResources::Texture2D *RendererBase::CreateTexture2D(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags, const std::string &name)
    {
        RenderResources::Texture2D* texture = new RenderResources::Texture2D(name);
        if (!texture->Create(width, height, levels, format, flags))
        {
            delete texture;
            return nullptr;
        }
        return texture;
    }

    RenderResources::IndexBuffer *RendererBase::CreateIndexBuffer(const void *data, uint32_t stride, uint32_t indexCount, const std::string &name, RHI::ERHIMemoryType memoryType)
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

        RenderResources::IndexBuffer* idBuffer = new RenderResources::IndexBuffer(name);
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

    RenderResources::StructuredBuffer *RendererBase::CreateStructuredBuffer(const void *data, uint32_t stride, uint32_t elementCount, const std::string &name, RHI::ERHIMemoryType memoryType, bool isUAV)
    {
        RenderResources::StructuredBuffer* pBuffer = new RenderResources::StructuredBuffer(name);
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

    inline void imageCopy(char* srcData, char* dstData, uint32_t srcRowPitch, uint32_t dstRowPitch, uint32_t rowNum, uint32_t d)
    {
        uint32_t srcSliceSize = srcRowPitch * rowNum;
        uint32_t dstSliceSize = dstRowPitch * rowNum;

        for (uint32_t z = 0; z < d; z++)
        {
            char* srcSlice = srcData + z * srcSliceSize;
            char* dstSlice = dstData + z * dstSliceSize;

            for (uint32_t row = 0; row < rowNum; row++)
            {
                memcpy(dstSlice + row * dstRowPitch, srcSlice + row * srcRowPitch, srcRowPitch);
            }
        }
    }

    void RendererBase::UploadTexture(RHI::RHITexture* pTexture, const void *pData)
    {
        uint32_t frameIndex = mpDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        StagingBufferAllocator* pAllocator = mpStagingBufferAllocators[frameIndex].get();

        uint32_t requiredSize = pTexture->GetRequiredStagingBufferSize();
        StagingBuffer buffer = pAllocator->Allocate(requiredSize);

        const RHI::RHITextureDesc& desc = pTexture->GetDesc();

        char* dstData = (char*)buffer.Buffer->GetCPUAddress() + buffer.Offset;
        uint32_t dstOffset = 0;
        uint32_t srcOffset = 0;

        const uint32_t minWidth = GetFormatBlockWidth(desc.Format);
        const uint32_t minHeight = GetFormatBlockHeight(desc.Format);
        const uint32_t alignment = mpDevice->GetDesc().RenderBackend == RHI::ERHIRenderBackend::D3D12 ? 512 : 1;

        for (uint32_t slice = 0; slice < desc.ArraySize; ++slice)
        {
            for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
            {
                uint32_t w = max(desc.Width >> mip, minWidth);
                uint32_t h = max(desc.Height >> mip, minHeight);
                uint32_t d = max(desc.Depth >> mip, 1u);

                uint32_t srcRowPitch = GetFormatRowPitch(desc.Format, w) * GetFormatBlockHeight(desc.Format);
                uint32_t dstRowPitch = pTexture->GetRowPitch(mip);

                uint32_t rowNum = h / GetFormatBlockHeight(desc.Format);

                imageCopy((char*)pData + srcOffset, dstData + dstOffset, srcRowPitch, dstRowPitch, rowNum, d);

                TextureUpload upload;
                upload.Texture = pTexture;
                upload.MipLevel = mip;
                upload.ArraySlice = slice;
                upload.Offset = dstOffset;
                upload.SBForUpload = buffer;
                mPendingTextureUpload.push_back(upload);

                srcOffset += srcRowPitch * rowNum;
                dstOffset += RoundUpPow2(dstRowPitch * rowNum, alignment);
            }
        }

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

    void RendererBase::SetupGlobalConstants(RHI::RHICommandList *pCmdList)
    {
        Scene::World* pWorld = Core::VultanaEngine::GetEngineInstance()->GetWorld();
        Scene::Camera* pCamera = pWorld->GetCamera();

        FSceneConstants sceneConstants {};
        pCamera->SetupCameraCB(sceneConstants.CameraCB);

        if (pCmdList->GetQueueType() == RHI::ERHICommandQueueType::Graphics)
        {
            // slot 0 only for instance data, must less than 8 * sizeof(uint32_t) = 32 bytes
            pCmdList->SetGraphicsConstants(1, &sceneConstants, sizeof(FSceneConstants));
        }
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
        std::vector<Vertex> triVertices = 
        {
            { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        };
        std::vector<uint16_t> triIndices = { 0, 1, 2 };

        std::vector<float3> boxPositions = 
        {
            // 前面
            { -0.5f, -0.5f, -0.5f }, // 左下前
            { -0.5f,  0.5f, -0.5f }, // 右下前
            {  0.5f,  0.5f, -0.5f }, // 右上前
            {  0.5f, -0.5f, -0.5f }, // 左上前

            // 后面
            { -0.5f, -0.5f,  0.5f }, // 左下后
            { -0.5f,  0.5f,  0.5f }, // 右下后
            {  0.5f,  0.5f,  0.5f }, // 右上后
            {  0.5f, -0.5f,  0.5f }, // 左上后
        };
        std::vector<float3> boxColors = 
        {
            // 前面
            { 1.0f, 0.0f, 0.0f }, // 左下前
            { 0.0f, 1.0f, 0.0f }, // 右下前
            { 0.0f, 0.0f, 1.0f }, // 右上前
            { 1.0f, 1.0f, 0.0f }, // 左上前

            // 后面
            { 0.0f, 1.0f, 1.0f }, // 左下后
            { 1.0f, 0.0f, 1.0f }, // 右下后
            { 1.0f, 1.0f, 1.0f }, // 右上后
            { 0.5f, 0.5f, 0.5f }, // 左上后
        };
    
        std::vector<uint32_t> boxIndices = 
        {
            // front face
		    0, 2, 1,
		    0, 3, 2,

		    // back face
		    4, 5, 6,
		    4, 6, 7,

		    // left face
		    4, 1, 5,
		    4, 0, 1,

		    // right face
		    3, 6, 2,
		    3, 7, 6,

		    // top face
		    1, 6, 5,
		    1, 2, 6,

		    // bottom face
		    4, 3, 0,
		    4, 7, 3
        };

        // mTestTriangleIndexBuffer.reset(CreateIndexBuffer(triIndices.data(), sizeof(uint16_t), static_cast<uint32_t>(triIndices.size()), "TriangleIndexBuffer"));
        // mTestTriangleVertexBuffer.reset(CreateStructuredBuffer(triVertices.data(), sizeof(Vertex), static_cast<uint32_t>(triVertices.size()), "TriangleVertexBuffer"));

        mTestBoxIndexBuffer.reset(CreateIndexBuffer(boxIndices.data(), sizeof(uint32_t), static_cast<uint32_t>(boxIndices.size()), "BoxIndexBuffer"));
        mTestBoxPositionBuffer.reset(CreateStructuredBuffer(boxPositions.data(), sizeof(float3), static_cast<uint32_t>(boxPositions.size()), "BoxPositionBuffer"));
        mTestBoxColorBuffer.reset(CreateStructuredBuffer(boxColors.data(), sizeof(float3), static_cast<uint32_t>(boxColors.size()), "BoxColorBuffer"));

        RHI::RHIGraphicsPipelineStateDesc psoDesc;
        // psoDesc.VS = GetShader("Triangle.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        // psoDesc.PS = GetShader("Triangle.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        psoDesc.VS = GetShader("Box.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        psoDesc.PS = GetShader("Box.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        psoDesc.DepthStencilState.bDepthWrite = true;
        psoDesc.DepthStencilState.bDepthTest = true;
        psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
        psoDesc.RasterizerState.bFrontCCW = true;
        psoDesc.RasterizerState.CullMode = RHI::ERHICullMode::Back;
        // psoDesc.RasterizerState.bWireFrame = true;
        psoDesc.RTFormats[0] = RHI::ERHIFormat::RGBA16F;
        psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

        // mTestPSO = GetPipelineState(psoDesc, "TrianglePSO");
        mTestPSO = GetPipelineState(psoDesc, "BoxPSO");

        RHI::RHIGraphicsPipelineStateDesc copyPSODesc;
        copyPSODesc.VS = GetShader("Copy.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        copyPSODesc.PS = GetShader("Copy.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        copyPSODesc.DepthStencilState.bDepthWrite = false;
        copyPSODesc.RTFormats[0] = mpSwapchain->GetDesc()->ColorFormat;
        copyPSODesc.DepthStencilFormat = RHI::ERHIFormat::D32F;
        mpCopyColorPSO = GetPipelineState(copyPSODesc, "CopyColorPSO");

        // copyPSODesc.PS = GetShader("Copy.hlsl", "PSMain", RHI::ERHIShaderType::PS, { "OUTPUT_DEPTH=1" });
        // copyPSODesc.DepthStencilState.bDepthWrite = true;
        // copyPSODesc.DepthStencilState.bDepthTest = true;
        // copyPSODesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::Always;
        // mpCopyColorDepthPSO = GetPipelineState(copyPSODesc, "CopyDepthPSO");

        // RHI::RHIComputePipelineStateDesc computePSODesc;
        // computePSODesc.CS = GetShader("Copy.hlsl", "CSCopyDepth", RHI::ERHIShaderType::CS);
        // mpCopyDepthPSO = GetPipelineState(computePSODesc, "CopyDepthComputePSO");
    }

    void RendererBase::OnWindowResize(void* wndHandle, uint32_t width, uint32_t height)
    {
        WaitGPU();
        
        if (mpSwapchain->GetDesc()->WindowHandle == wndHandle)
        {
            mpSwapchain->Resize(width, height);
            mDisplayWidth = width;
            mDisplayHeight = height;
            mRenderWidth = mDisplayWidth;
            mRenderHeight = mDisplayHeight;
        }
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
        if (mPendingTextureUpload.empty() && mPendingBufferUpload.empty()) return;

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

            for (size_t i = 0; i < mPendingTextureUpload.size(); i++)
            {
                const TextureUpload& upload = mPendingTextureUpload[i];
                pUploadeCmdList->CopyBufferToTexture(upload.SBForUpload.Buffer, upload.Texture, upload.MipLevel, upload.ArraySlice, upload.SBForUpload.Offset + upload.Offset);
            }
        }

        pUploadeCmdList->End();
        pUploadeCmdList->Signal(mpUploadFence.get(), ++mCurrentUploadFenceValue);
        pUploadeCmdList->Submit();

        RHI::RHICommandList* pCmdList = mpCmdList[frameIndex].get();
        pCmdList->Wait(mpUploadFence.get(), mCurrentUploadFenceValue);

        if (mpDevice->GetDesc().RenderBackend == RHI::ERHIRenderBackend::Vulkan)
        {
            for (size_t i = 0; i < mPendingTextureUpload.size(); i++)
            {
                const TextureUpload& upload = mPendingTextureUpload[i];
                pCmdList->TextureBarrier(upload.Texture, CalcSubresource(upload.Texture->GetDesc(), upload.MipLevel, upload.ArraySlice), RHI::RHIAccessCopyDst, RHI::RHIAccessMaskSRV);
            }
        }

        mPendingBufferUpload.clear();
        mPendingTextureUpload.clear();
    }

    void RendererBase::Render()
    {
        uint32_t frameIndex = mpDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHICommandList* pCmdList = mpCmdList[frameIndex].get();
        RHI::RHICommandList* pComputeCmdList = mpAsyncComputeCmdList[frameIndex].get();

        SetupGlobalConstants(pCmdList);

        {
            GPU_EVENT_DEBUG(pCmdList, "RenderTriangle");

            RHI::RHIRenderPassDesc renderPassDesc {};
            renderPassDesc.Color[0].Texture = mpTestRT->GetTexture();
            renderPassDesc.Color[0].LoadOp = RHI::ERHIRenderPassLoadOp::Clear;
            renderPassDesc.Color[0].ClearColor[0] = 0.1f;
            renderPassDesc.Color[0].ClearColor[1] = 0.1f;
            renderPassDesc.Color[0].ClearColor[2] = 0.1f;
            renderPassDesc.Color[0].ClearColor[3] = 1.0f;
            renderPassDesc.Depth.Texture = mpTestDepthRT->GetTexture();
            renderPassDesc.Depth.DepthLoadOp = RHI::ERHIRenderPassLoadOp::Clear;
            renderPassDesc.Depth.StencilLoadOp = RHI::ERHIRenderPassLoadOp::Clear;
            pCmdList->BeginRenderPass(renderPassDesc);

            pCmdList->SetPipelineState(mTestPSO);

            // pCmdList->SetIndexBuffer(mTestIndexBuffer->GetBuffer(), 0, mTestIndexBuffer->GetFormat());
            // uint32_t vertexBuffer = mTestVertexBuffer->GetSRV()->GetHeapIndex();
            // pCmdList->SetGraphicsConstants(0, &vertexBuffer, sizeof(vertexBuffer));
            pCmdList->SetIndexBuffer(mTestBoxIndexBuffer->GetBuffer(), 0, mTestBoxIndexBuffer->GetFormat());

            uint32_t vertexCB[2] = 
            {
                mTestBoxPositionBuffer->GetSRV()->GetHeapIndex(),
                mTestBoxColorBuffer->GetSRV()->GetHeapIndex()
            };
            pCmdList->SetGraphicsConstants(0, &vertexCB, sizeof(vertexCB));

            pCmdList->DrawIndexed(mTestBoxIndexBuffer->GetIndexCount());

            pCmdList->EndRenderPass();
        }

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

        mpStagingBufferAllocators[frameIndex]->Reset();

        mpDevice->EndFrame();
    }

    void RendererBase::RenderBackBufferPass(RHI::RHICommandList *pCmdList)
    {
        mpSwapchain->AcquireNextBackBuffer();
        pCmdList->TextureBarrier(mpSwapchain->GetBackBuffer(), 0, RHI::RHIAccessPresent, RHI::RHIAccessRTV);

        {
            GPU_EVENT_DEBUG(pCmdList, "RenderBackBufferPass");

            RHI::RHIRenderPassDesc renderPassDesc {};
            renderPassDesc.Color[0].Texture = mpSwapchain->GetBackBuffer();
            renderPassDesc.Color[0].LoadOp = RHI::ERHIRenderPassLoadOp::DontCare;
            pCmdList->BeginRenderPass(renderPassDesc);

            uint32_t constants[2] = 
            {
                mpTestRT->GetSRV()->GetHeapIndex(),
                mpPointSampler->GetHeapIndex()
            };
            pCmdList->SetGraphicsConstants(0, constants, sizeof(constants));
            pCmdList->SetPipelineState(mpCopyColorPSO);
            pCmdList->Draw(3);

            Core::VultanaEngine::GetEngineInstance()->GetGUI()->Render(pCmdList);

            pCmdList->EndRenderPass();
        }

        pCmdList->TextureBarrier(mpSwapchain->GetBackBuffer(), 0, RHI::RHIAccessRTV, RHI::RHIAccessPresent);
    }
} // namespace Vultana::Renderer
