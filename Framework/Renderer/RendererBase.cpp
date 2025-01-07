#include "RendererBase.hpp"
#include "GPUScene.hpp"
#include "PipelineStateCache.hpp"
#include "ShaderCompiler.hpp"
#include "ShaderCache.hpp"
#include "RHI/RHI.hpp"
#include "Core/VultanaEngine.hpp"
#include "Core/ImGUIImplement.hpp"
#include "Utilities/Log.hpp"
#include "Window/GLFWindow.hpp"
#include "AssetManager/TextureLoader.hpp"
#include "ForwardPath/ForwardBasePass.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

namespace Renderer
{
    RendererBase::RendererBase()
    {
        mpShaderCache = eastl::make_unique<ShaderCache>(this);
        mpShaderCompiler = eastl::make_unique<ShaderCompiler>(this);
        mpPipelineStateCache = eastl::make_unique<PipelineStateCache>(this);
        mCBAllocator = eastl::make_unique<LinearAllocator>(1024 * 1024 * 8);
        
        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.connect(&RendererBase::OnWindowResize, this);
    }

    RendererBase::~RendererBase()
    {
        WaitGPU();

        if (mpRenderGraph)
        {
            mpRenderGraph->Clear();
        }

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
            eastl::string name = fmt::format("RendererBase::CommonCmdList{}", i).c_str();
            mpCmdList[i].reset(mpDevice->CreateCommandList(RHI::ERHICommandQueueType::Graphics, name));
        }

        mpAsyncComputeFence.reset(mpDevice->CreateFence("RendererBase::AsyncComputeFence"));
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            eastl::string name = fmt::format("RendererBase::AsyncComputeCmdList{}", i).c_str();
            mpAsyncComputeCmdList[i].reset(mpDevice->CreateCommandList(RHI::ERHICommandQueueType::Compute, name));
        }

        mpUploadFence.reset(mpDevice->CreateFence("RendererBase::UploadFence"));
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            eastl::string name = fmt::format("RendererBase::UploadCmdList{}", i).c_str();
            mpUploadCmdList[i].reset(mpDevice->CreateCommandList(RHI::ERHICommandQueueType::Copy, name));
            mpStagingBufferAllocators[i] = eastl::make_unique<StagingBufferAllocator>(this);
        }

        CreateCommonResources();

        mpRenderGraph = eastl::make_unique<RG::RenderGraph>(this);
        mpGPUScene = eastl::make_unique<GPUScene>(this);
        mpForwardBasePass = eastl::make_unique<ForwardBasePass>(this);
        
        return true;
    }

    void RendererBase::RenderFrame()
    {
        mpGPUScene->Update();

        BuildRenderGraph(mOutputColorHandle, mOutputDepthHandle);

        BeginFrame();
        UploadResource();
        Render();
        EndFrame();

        MouseHitTest();
    }

    void RendererBase::WaitGPU()
    {
        if (mpFrameFence)
        {
            mpFrameFence->Wait(mCurrentFrameFenceValue);
        }
    }

    RHI::RHIShader *RendererBase::GetShader(const eastl::string &file, const eastl::string &entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string> &defines, RHI::ERHIShaderCompileFlags flags)
    {
        return mpShaderCache->GetShader(file, entryPoint, type, defines, flags);
    }

    RHI::RHIPipelineState *RendererBase::GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc &desc, const eastl::string &name)
    {
        return mpPipelineStateCache->GetPipelineState(desc, name);
    }

    RHI::RHIPipelineState *RendererBase::GetPipelineState(const RHI::RHIComputePipelineStateDesc &desc, const eastl::string &name)
    {
        return mpPipelineStateCache->GetPipelineState(desc, name);
    }

    void RendererBase::ReloadShaders()
    {
        mpShaderCache->ReloadShaders();
    }

    RenderResources::Texture2D *RendererBase::CreateTexture2D(const eastl::string &file, bool srgb)
    {
        Assets::TextureLoader loader;
        if (!loader.Load(file, srgb))
        {
            return nullptr;
        }
        RenderResources::Texture2D* texture = CreateTexture2D(loader.GetWidth(), loader.GetHeight(), loader.GetMipLevels(), loader.GetFormat(), 0, file);
        if (texture)
        {
            UploadTexture(texture->GetTexture(), loader.GetData());
        }
        return texture;
    }

    RenderResources::Texture2D *RendererBase::CreateTexture2D(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags, const eastl::string &name)
    {
        RenderResources::Texture2D* texture = new RenderResources::Texture2D(name);
        if (!texture->Create(width, height, levels, format, flags))
        {
            delete texture;
            return nullptr;
        }
        return texture;
    }

    RenderResources::IndexBuffer *RendererBase::CreateIndexBuffer(const void *data, uint32_t stride, uint32_t indexCount, const eastl::string &name, RHI::ERHIMemoryType memoryType)
    {
        eastl::vector<uint16_t> u16IndexBuffer;
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

    RenderResources::StructuredBuffer *RendererBase::CreateStructuredBuffer(const void *data, uint32_t stride, uint32_t elementCount, const eastl::string &name, RHI::ERHIMemoryType memoryType, bool isUAV)
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

    RenderResources::RawBuffer *RendererBase::CreateRawBuffer(const void *data, uint32_t size, const eastl::string &name, RHI::ERHIMemoryType memoryType, bool isUAV)
    {
        auto buffer = new RenderResources::RawBuffer(name);
        if (!buffer->Create(size, memoryType, isUAV))
        {
            delete buffer;
            return nullptr;
        }
        if (data)
        {
            UploadBuffer(buffer->GetBuffer(), data, 0, size);
        }
        return buffer;
    }

    RHI::RHIBuffer *RendererBase::GetSceneStaticBuffer() const
    {
        return mpGPUScene->GetSceneStaticBuffer();
    }

    OffsetAllocator::Allocation RendererBase::AllocateSceneStaticBuffer(const void *data, uint32_t size)
    {
        OffsetAllocator::Allocation allocation = mpGPUScene->AllocateStaticBuffer(size);
        if (data)
        {
            UploadBuffer(mpGPUScene->GetSceneStaticBuffer(), data, allocation.offset, size);
        }
        return allocation;
    }

    void RendererBase::FreeSceneStaticBuffer(OffsetAllocator::Allocation allocation)
    {
        mpGPUScene->FreeStaticBuffer(allocation);
    }

    RHI::RHIBuffer *RendererBase::GetSceneAnimationBuffer() const
    {
        return mpGPUScene->GetSceneAnimationBuffer();
    }

    OffsetAllocator::Allocation RendererBase::AllocateSceneAnimationBuffer(uint32_t size)
    {
        return mpGPUScene->AllocateAnimationBuffer(size);
    }

    void RendererBase::FreeSceneAnimationBuffer(OffsetAllocator::Allocation allocation)
    {
        mpGPUScene->FreeAnimationBuffer(allocation);
    }

    uint32_t RendererBase::AllocateSceneConstantBuffer(const void *data, uint32_t size)
    {
        uint32_t address = mpGPUScene->AllocateConstantBuffer(size);
        if (data)
        {
            void* dst = (char*)mpGPUScene->GetSceneConstantBuffer()->GetCPUAddress() + address;
            memcpy(dst, data, size);
        }
        return address;
    }

    uint32_t RendererBase::AddInstance(const FInstanceData &instanceData)
    {
        return mpGPUScene->AddInstance(instanceData);
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
        Scene::ILight* pMainLight = pWorld->GetMainLight();

        FSceneConstants sceneConstants {};
        pCamera->SetupCameraCB(sceneConstants.CameraCB);
        sceneConstants.SceneConstantBufferSRV = mpGPUScene->GetSceneConstantBufferSRV()->GetHeapIndex();
        sceneConstants.SceneStaticBufferSRV = mpGPUScene->GetSceneStaticBufferSRV()->GetHeapIndex();
        sceneConstants.SceneAnimationBufferSRV = mpGPUScene->GetSceneAnimationBufferSRV()->GetHeapIndex();
        sceneConstants.SceneAnimationBufferUAV = mpGPUScene->GetSceneAnimationBufferUAV()->GetHeapIndex();
        sceneConstants.instanceDataAddress = mpGPUScene->GetInstanceDataAddress();
        sceneConstants.LightColor = pMainLight->GetLightColor();
        sceneConstants.LightDirection = pMainLight->GetLightDirection();
        sceneConstants.LightRadius = pMainLight->GetLightRadius();

        sceneConstants.RenderSize = uint2(mRenderWidth, mRenderHeight);
        sceneConstants.RenderSizeInv = float2(1.0f / mRenderWidth, 1.0f / mRenderHeight);
        sceneConstants.DisplaySize = uint2(mDisplayWidth, mDisplayHeight);
        sceneConstants.DisplaySizeInv = float2(1.0f / mDisplayWidth, 1.0f / mDisplayHeight);

        sceneConstants.PointRepeatSampler = mpPointRepeatSampler->GetHeapIndex();
        sceneConstants.PointClampSampler = mpPointClampSampler->GetHeapIndex();
        sceneConstants.BilinearRepeatSampler = mpBilinearRepeatSampler->GetHeapIndex();
        sceneConstants.BilinearClampSampler = mpBilinearClampSampler->GetHeapIndex();
        sceneConstants.TrilinearRepeatSampler = mpTrilinearRepeatSampler->GetHeapIndex();
        sceneConstants.TrilinearClampSampler = mpTrilinearClampSampler->GetHeapIndex();
        sceneConstants.Aniso2xSampler = mpAniso2xSampler->GetHeapIndex();
        sceneConstants.Aniso4xSampler = mpAniso4xSampler->GetHeapIndex();
        sceneConstants.Aniso8xSampler = mpAniso8xSampler->GetHeapIndex();
        sceneConstants.Aniso16xSampler = mpAniso16xSampler->GetHeapIndex();

        sceneConstants.FrameTime = Core::VultanaEngine::GetEngineInstance()->GetDeltaTime();
        sceneConstants.FrameIndex = (uint32_t)GetFrameID();

        if (pCmdList->GetQueueType() == RHI::ERHICommandQueueType::Graphics)
        {
            // slot 0 only for instance data, must less than 8 * sizeof(uint32_t) = 32 bytes
            pCmdList->SetGraphicsConstants(2, &sceneConstants, sizeof(FSceneConstants));
        }
        pCmdList->SetComputeConstants(2, &sceneConstants, sizeof(FSceneConstants));
    }

    RenderBatch &RendererBase::AddBasePassBatch()
    {
        return mpForwardBasePass->AddBatch();
    }

    void RendererBase::RequestMouseHitTest(uint32_t x, uint32_t y)
    {
        mMouseX = x;
        mMouseY = y;
        mbEnableObjectIDRendering = true;
    }

    void RendererBase::CreateCommonResources()
    {
        RHI::RHISamplerDesc samplerDesc {};
        mpPointRepeatSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::PointRepeatSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MagFilter = RHI::ERHIFilter::Linear;
        mpBilinearRepeatSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::BilinearRepeatSampler"));

        samplerDesc.MipFilter = RHI::ERHIFilter::Linear;
        mpTrilinearRepeatSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::TrilinearRepeatSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Point;
        samplerDesc.MagFilter = RHI::ERHIFilter::Point;
        samplerDesc.MipFilter = RHI::ERHIFilter::Point;
        samplerDesc.AddressU = RHI::ERHISamplerAddressMode::ClampToEdge;
        samplerDesc.AddressV = RHI::ERHISamplerAddressMode::ClampToEdge;
        samplerDesc.AddressW = RHI::ERHISamplerAddressMode::ClampToEdge;
        mpPointClampSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::PointClampSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MagFilter = RHI::ERHIFilter::Linear;
        mpBilinearClampSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::BilinearClampSampler"));

        samplerDesc.MipFilter = RHI::ERHIFilter::Linear;
        mpTrilinearClampSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::TrilinearClampSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MagFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MipFilter = RHI::ERHIFilter::Linear;
        samplerDesc.AddressU = RHI::ERHISamplerAddressMode::Repeat;
        samplerDesc.AddressV = RHI::ERHISamplerAddressMode::Repeat;
        samplerDesc.AddressW = RHI::ERHISamplerAddressMode::Repeat;
        samplerDesc.bEnableAnisotropy = true;
        samplerDesc.MaxAnisotropy = 2.0f;
        mpAniso2xSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::Aniso2xSampler"));

        samplerDesc.MaxAnisotropy = 4.0f;
        mpAniso4xSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::Aniso4xSampler"));

        samplerDesc.MaxAnisotropy = 8.0f;
        mpAniso8xSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::Aniso8xSampler"));

        samplerDesc.MaxAnisotropy = 16.0f;
        mpAniso16xSampler.reset(mpDevice->CreateSampler(samplerDesc, "RendererBase::Aniso16xSampler"));

        RHI::RHIGraphicsPipelineStateDesc copyPSODesc;
        copyPSODesc.VS = GetShader("Copy.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        copyPSODesc.PS = GetShader("Copy.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        copyPSODesc.DepthStencilState.bDepthWrite = false;
        copyPSODesc.RTFormats[0] = mpSwapchain->GetDesc()->ColorFormat;
        copyPSODesc.DepthStencilFormat = RHI::ERHIFormat::D32F;
        mpCopyColorPSO = GetPipelineState(copyPSODesc, "CopyColorPSO");

        // copyPSODesc.PS = GetShader("Copy.hlsl", "PSMain", RHI::ERHIShaderType::PS, { "OUTPUT_DEPTH = 1"});
        // copyPSODesc.DepthStencilState.bDepthWrite = true;
        // copyPSODesc.DepthStencilState.bDepthTest = true;
        // copyPSODesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::Always;
        // mpCopyColorDepthPSO = GetPipelineState(copyPSODesc, "CopyColorDepthPSO");
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
        pCmdList->ResetAllocator();
        pCmdList->Begin();

        RHI::RHICommandList* pComputeCmdList = mpAsyncComputeCmdList[frameIndex].get();
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
            GPU_EVENT_DEBUG(pUploadeCmdList, "RendererBase::UploadResources");
            
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

        GPU_EVENT_DEBUG(pCmdList, fmt::format("Render Frame {}", mpDevice->GetFrameID()).c_str());

        SetupGlobalConstants(pCmdList);
        FlushComputePass(pCmdList);

        mpRenderGraph->Execute(this, pCmdList, pComputeCmdList);
        
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
        mCBAllocator->Reset();
        mpGPUScene->ResetFrameData();

        mAnimationBatches.clear();
        mIDPassBatches.clear();
        mOutlinePassBatches.clear();

        mpDevice->EndFrame();
    }

    void RendererBase::FlushComputePass(RHI::RHICommandList *pCmdList)
    {
        if (!mAnimationBatches.empty())
        {
            GPU_EVENT_DEBUG(pCmdList, "Animation Pass");
            
            mpGPUScene->BeginAnimationUpdate(pCmdList);
            for (size_t i = 0; i < mAnimationBatches.size(); i++)
            {
                DispatchComputeBatch(pCmdList, mAnimationBatches[i]);
            }
            mpGPUScene->EndAnimationUpdate(pCmdList);
        }
    }

    void RendererBase::RenderBackBufferPass(RHI::RHICommandList *pCmdList)
    {
        mpSwapchain->AcquireNextBackBuffer();
        pCmdList->TextureBarrier(mpSwapchain->GetBackBuffer(), 0, RHI::RHIAccessPresent, RHI::RHIAccessRTV);

        RG::RGTexture* pColorRT = mpRenderGraph->GetTexture(mOutputColorHandle);
        RG::RGTexture* pDepthRT = mpRenderGraph->GetTexture(mOutputDepthHandle);

        {
            GPU_EVENT_DEBUG(pCmdList, "RenderBackBufferPass");

            RHI::RHIRenderPassDesc renderPassDesc {};
            renderPassDesc.Color[0].Texture = mpSwapchain->GetBackBuffer();
            renderPassDesc.Color[0].LoadOp = RHI::ERHIRenderPassLoadOp::DontCare;
            renderPassDesc.Depth.Texture = pDepthRT->GetTexture();
            renderPassDesc.Depth.DepthLoadOp = RHI::ERHIRenderPassLoadOp::Load;
            renderPassDesc.Depth.DepthStoreOp = RHI::ERHIRenderPassStoreOp::DontCare;
            renderPassDesc.Depth.StencilLoadOp = RHI::ERHIRenderPassLoadOp::DontCare;
            renderPassDesc.Depth.StencilStoreOp = RHI::ERHIRenderPassStoreOp::DontCare;
            renderPassDesc.Depth.bReadOnly = true;
            pCmdList->BeginRenderPass(renderPassDesc);

            uint32_t constants[3] = 
            {
                pColorRT->GetSRV()->GetHeapIndex(),
                pDepthRT->GetSRV()->GetHeapIndex(),
                mpPointRepeatSampler->GetHeapIndex()
            };
            pCmdList->SetGraphicsConstants(0, constants, sizeof(constants));
            pCmdList->SetPipelineState(mpCopyColorPSO);
            pCmdList->Draw(3);

            for (size_t i = 0; i < mGUIBatches.size(); i++)
            {
                DrawBatch(pCmdList, mGUIBatches[i]);
            }

            Core::VultanaEngine::GetEngineInstance()->GetEditor()->Render(pCmdList);

            pCmdList->EndRenderPass();
        }
        pCmdList->TextureBarrier(mpSwapchain->GetBackBuffer(), 0, RHI::RHIAccessRTV, RHI::RHIAccessPresent);
    }

    void RendererBase::MouseHitTest()
    {
        if (mbEnableObjectIDRendering)
        {
            WaitGPU();
            
            uint32_t x = mMouseX;
            uint32_t y = mMouseY;

            if (mpObjectIDBuffer == nullptr) return;
            uint8_t* data = (uint8_t*)mpObjectIDBuffer->GetCPUAddress();
            uint32_t dataOffset = mObjectIDRowPitch * y + x * sizeof(uint32_t);
            memcpy(&mMouseHitObjectID, data + dataOffset, sizeof(uint32_t));

            mbEnableObjectIDRendering = false;
        }
    }
} // namespace Vultana::Renderer
