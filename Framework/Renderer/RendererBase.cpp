#include "RendererBase.hpp"
#include "GPUScene.hpp"
#include "PipelineStateCache.hpp"
#include "ShaderCompiler.hpp"
#include "ShaderCache.hpp"
#include "RHI/RHI.hpp"
#include "Core/VultanaEngine.hpp"
#include "Editor/ImGUIImplement.hpp"
#include "Utilities/Log.hpp"
#include "Window/GLFWindow.hpp"
#include "AssetManager/TextureLoader.hpp"
#include "ForwardPath/ForwardBasePass.hpp"
#include "RenderModules/GPUDrivenDebugLine.hpp"
#include "Common/GlobalConstants.hlsli"

#include <optional>
#include <algorithm>
#include <fstream>

namespace Renderer
{
    RendererBase::RendererBase()
    {
        m_pShaderCache = eastl::make_unique<ShaderCache>(this);
        m_pShaderCompiler = eastl::make_unique<ShaderCompiler>(this);
        m_pPipelineStateCache = eastl::make_unique<PipelineStateCache>(this);
        m_CBAllocator = eastl::make_unique<LinearAllocator>(1024 * 1024 * 8);
        
        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.connect(&RendererBase::OnWindowResize, this);
    }

    RendererBase::~RendererBase()
    {
        WaitGPU();

        if (m_pRenderGraph)
        {
            m_pRenderGraph->Clear();
        }

        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.disconnect(this);
    }

    bool RendererBase::CreateDevice(RHI::ERHIRenderBackend backend, void *windowHandle, uint32_t width, uint32_t height)
    {
        m_DisplayWidth = width;
        m_DisplayHeight = height;
        m_RenderWidth = width;
        m_RenderHeight = height;

        RHI::RHIDeviceDesc deviceDesc {};
        deviceDesc.RenderBackend = backend;
        m_pDevice.reset(RHI::CreateRHIDevice(deviceDesc));
        if (m_pDevice == nullptr)
        {
            VTNA_LOG_ERROR("[Renderer::CreateDevice] failed to create the RHI device.");
            return false;
        }

        RHI::RHISwapchainDesc swapchainDesc {};
        swapchainDesc.Width = width;
        swapchainDesc.Height = height;
        swapchainDesc.WindowHandle = windowHandle;
        m_pSwapchain.reset(m_pDevice->CreateSwapchain(swapchainDesc, "RendererBase::Swapchain"));
        
        m_pFrameFence.reset(m_pDevice->CreateFence("RendererBase::FrameFence"));
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            eastl::string name = fmt::format("RendererBase::CommonCmdList{}", i).c_str();
            m_pCmdList[i].reset(m_pDevice->CreateCommandList(RHI::ERHICommandQueueType::Graphics, name));
        }

        m_pAsyncComputeFence.reset(m_pDevice->CreateFence("RendererBase::AsyncComputeFence"));
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            eastl::string name = fmt::format("RendererBase::AsyncComputeCmdList{}", i).c_str();
            m_pAsyncComputeCmdList[i].reset(m_pDevice->CreateCommandList(RHI::ERHICommandQueueType::Compute, name));
        }

        m_pUploadFence.reset(m_pDevice->CreateFence("RendererBase::UploadFence"));
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            eastl::string name = fmt::format("RendererBase::UploadCmdList{}", i).c_str();
            m_pUploadCmdList[i].reset(m_pDevice->CreateCommandList(RHI::ERHICommandQueueType::Copy, name));
            m_pStagingBufferAllocators[i] = eastl::make_unique<StagingBufferAllocator>(this);
        }

        CreateCommonResources();

        m_pRenderGraph = eastl::make_unique<RG::RenderGraph>(this);
        m_pGPUScene = eastl::make_unique<GPUScene>(this);
        m_pForwardBasePass = eastl::make_unique<ForwardBasePass>(this);
        m_pGPUDrivenDebugLine = eastl::make_unique<GPUDrivenDebugLine>(this);
        
        return true;
    }

    void RendererBase::RenderFrame()
    {
        m_pGPUScene->Update();

        BuildRenderGraph(m_OutputColorHandle, m_OutputDepthHandle);

        BeginFrame();
        UploadResource();
        Render();
        EndFrame();

        MouseHitTest();
    }

    void RendererBase::WaitGPU()
    {
        if (m_pFrameFence)
        {
            m_pFrameFence->Wait(m_CurrentFrameFenceValue);
        }
    }

    RHI::RHIShader *RendererBase::GetShader(const eastl::string &file, const eastl::string &entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string> &defines, RHI::ERHIShaderCompileFlags flags)
    {
        return m_pShaderCache->GetShader(file, entryPoint, type, defines, flags);
    }

    RHI::RHIPipelineState *RendererBase::GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc &desc, const eastl::string &name)
    {
        return m_pPipelineStateCache->GetPipelineState(desc, name);
    }

    RHI::RHIPipelineState *RendererBase::GetPipelineState(const RHI::RHIMeshShadingPipelineStateDesc &desc, const eastl::string &name)
    {
        return m_pPipelineStateCache->GetPipelineState(desc, name);
    }

    RHI::RHIPipelineState *RendererBase::GetPipelineState(const RHI::RHIComputePipelineStateDesc &desc, const eastl::string &name)
    {
        return m_pPipelineStateCache->GetPipelineState(desc, name);
    }

    void RendererBase::ReloadShaders()
    {
        m_pShaderCache->ReloadShaders();
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
        return m_pGPUScene->GetSceneStaticBuffer();
    }

    OffsetAllocator::Allocation RendererBase::AllocateSceneStaticBuffer(const void *data, uint32_t size)
    {
        OffsetAllocator::Allocation allocation = m_pGPUScene->AllocateStaticBuffer(size);
        if (data)
        {
            UploadBuffer(m_pGPUScene->GetSceneStaticBuffer(), data, allocation.offset, size);
        }
        return allocation;
    }

    void RendererBase::FreeSceneStaticBuffer(OffsetAllocator::Allocation allocation)
    {
        m_pGPUScene->FreeStaticBuffer(allocation);
    }

    RHI::RHIBuffer *RendererBase::GetSceneAnimationBuffer() const
    {
        return m_pGPUScene->GetSceneAnimationBuffer();
    }

    OffsetAllocator::Allocation RendererBase::AllocateSceneAnimationBuffer(uint32_t size)
    {
        return m_pGPUScene->AllocateAnimationBuffer(size);
    }

    void RendererBase::FreeSceneAnimationBuffer(OffsetAllocator::Allocation allocation)
    {
        m_pGPUScene->FreeAnimationBuffer(allocation);
    }

    uint32_t RendererBase::AllocateSceneConstantBuffer(const void *data, uint32_t size)
    {
        uint32_t address = m_pGPUScene->AllocateConstantBuffer(size);
        if (data)
        {
            void* dst = (char*)m_pGPUScene->GetSceneConstantBuffer()->GetCPUAddress() + address;
            memcpy(dst, data, size);
        }
        return address;
    }

    uint32_t RendererBase::AddInstance(const FInstanceData &instanceData)
    {
        return m_pGPUScene->AddInstance(instanceData);
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
        uint32_t frameIndex = m_pDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        StagingBufferAllocator* pAllocator = m_pStagingBufferAllocators[frameIndex].get();

        uint32_t requiredSize = pTexture->GetRequiredStagingBufferSize();
        StagingBuffer buffer = pAllocator->Allocate(requiredSize);

        const RHI::RHITextureDesc& desc = pTexture->GetDesc();

        char* dstData = (char*)buffer.Buffer->GetCPUAddress() + buffer.Offset;
        uint32_t dstOffset = 0;
        uint32_t srcOffset = 0;

        const uint32_t minWidth = GetFormatBlockWidth(desc.Format);
        const uint32_t minHeight = GetFormatBlockHeight(desc.Format);
        const uint32_t alignment = m_pDevice->GetDesc().RenderBackend == RHI::ERHIRenderBackend::D3D12 ? 512 : 1;

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
                m_PendingTextureUpload.push_back(upload);

                srcOffset += srcRowPitch * rowNum;
                dstOffset += RoundUpPow2(dstRowPitch * rowNum, alignment);
            }
        }

    }

    void RendererBase::UploadBuffer(RHI::RHIBuffer *pBuffer, const void *pData, uint32_t offset, uint32_t dataSize)
    {
        uint32_t frameIndex = m_pDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        StagingBufferAllocator* pAllocator = m_pStagingBufferAllocators[frameIndex].get();

        StagingBuffer stagingBuffer = pAllocator->Allocate(dataSize);

        char* dstData = (char*)stagingBuffer.Buffer->GetCPUAddress() + stagingBuffer.Offset;
        memcpy(dstData, pData, dataSize);

        BufferUpload upload;
        upload.Buffer = pBuffer;
        upload.Offset = offset;
        upload.SBForUpload = stagingBuffer;
        m_PendingBufferUpload.push_back(upload);
    }

    void RendererBase::SetupGlobalConstants(RHI::RHICommandList *pCmdList)
    {
        Scene::World* pWorld = Core::VultanaEngine::GetEngineInstance()->GetWorld();
        Scene::Camera* pCamera = pWorld->GetCamera();
        Scene::ILight* pMainLight = pWorld->GetMainLight();

        FSceneConstants sceneConstants {};
        pCamera->SetupCameraCB(sceneConstants.CameraCB);
        sceneConstants.SceneConstantBufferSRV = m_pGPUScene->GetSceneConstantBufferSRV()->GetHeapIndex();
        sceneConstants.SceneStaticBufferSRV = m_pGPUScene->GetSceneStaticBufferSRV()->GetHeapIndex();
        sceneConstants.SceneAnimationBufferSRV = m_pGPUScene->GetSceneAnimationBufferSRV()->GetHeapIndex();
        sceneConstants.SceneAnimationBufferUAV = m_pGPUScene->GetSceneAnimationBufferUAV()->GetHeapIndex();
        sceneConstants.instanceDataAddress = m_pGPUScene->GetInstanceDataAddress();
        sceneConstants.LightColor = pMainLight->GetLightColor();
        sceneConstants.LightDirection = pMainLight->GetLightDirection();
        sceneConstants.LightRadius = pMainLight->GetLightRadius();

        sceneConstants.RenderSize = uint2(m_RenderWidth, m_RenderHeight);
        sceneConstants.RenderSizeInv = float2(1.0f / m_RenderWidth, 1.0f / m_RenderHeight);
        sceneConstants.DisplaySize = uint2(m_DisplayWidth, m_DisplayHeight);
        sceneConstants.DisplaySizeInv = float2(1.0f / m_DisplayWidth, 1.0f / m_DisplayHeight);

        sceneConstants.PrevSceneDepthSRV = m_pPrevSceneDepthTexture->GetSRV()->GetHeapIndex();
        sceneConstants.PrevSceneColorSRV = m_pPrevSceneColorTexture->GetSRV()->GetHeapIndex();

        sceneConstants.DebugLineDrawCommandUAV = m_pGPUDrivenDebugLine->GetDrawArgsBufferUAV()->GetHeapIndex();
        sceneConstants.DebugLineVertexBufferUAV = m_pGPUDrivenDebugLine->GetVertexBufferUAV()->GetHeapIndex();

        sceneConstants.PointRepeatSampler = m_pPointRepeatSampler->GetHeapIndex();
        sceneConstants.PointClampSampler = m_pPointClampSampler->GetHeapIndex();
        sceneConstants.BilinearRepeatSampler = m_pBilinearRepeatSampler->GetHeapIndex();
        sceneConstants.BilinearClampSampler = m_pBilinearClampSampler->GetHeapIndex();
        sceneConstants.TrilinearRepeatSampler = m_pTrilinearRepeatSampler->GetHeapIndex();
        sceneConstants.TrilinearClampSampler = m_pTrilinearClampSampler->GetHeapIndex();
        sceneConstants.Aniso2xSampler = m_pAniso2xSampler->GetHeapIndex();
        sceneConstants.Aniso4xSampler = m_pAniso4xSampler->GetHeapIndex();
        sceneConstants.Aniso8xSampler = m_pAniso8xSampler->GetHeapIndex();
        sceneConstants.Aniso16xSampler = m_pAniso16xSampler->GetHeapIndex();

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
        return m_pForwardBasePass->AddBatch();
    }

    void RendererBase::RequestMouseHitTest(uint32_t x, uint32_t y)
    {
        m_MouseX = x;
        m_MouseY = y;
        m_bEnableObjectIDRendering = true;
    }

    void RendererBase::CreateCommonResources()
    {
        RHI::RHISamplerDesc samplerDesc {};
        m_pPointRepeatSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::PointRepeatSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MagFilter = RHI::ERHIFilter::Linear;
        m_pBilinearRepeatSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::BilinearRepeatSampler"));

        samplerDesc.MipFilter = RHI::ERHIFilter::Linear;
        m_pTrilinearRepeatSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::TrilinearRepeatSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Point;
        samplerDesc.MagFilter = RHI::ERHIFilter::Point;
        samplerDesc.MipFilter = RHI::ERHIFilter::Point;
        samplerDesc.AddressU = RHI::ERHISamplerAddressMode::ClampToEdge;
        samplerDesc.AddressV = RHI::ERHISamplerAddressMode::ClampToEdge;
        samplerDesc.AddressW = RHI::ERHISamplerAddressMode::ClampToEdge;
        m_pPointClampSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::PointClampSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MagFilter = RHI::ERHIFilter::Linear;
        m_pBilinearClampSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::BilinearClampSampler"));

        samplerDesc.MipFilter = RHI::ERHIFilter::Linear;
        m_pTrilinearClampSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::TrilinearClampSampler"));

        samplerDesc.MinFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MagFilter = RHI::ERHIFilter::Linear;
        samplerDesc.MipFilter = RHI::ERHIFilter::Linear;
        samplerDesc.AddressU = RHI::ERHISamplerAddressMode::Repeat;
        samplerDesc.AddressV = RHI::ERHISamplerAddressMode::Repeat;
        samplerDesc.AddressW = RHI::ERHISamplerAddressMode::Repeat;
        samplerDesc.bEnableAnisotropy = true;
        samplerDesc.MaxAnisotropy = 2.0f;
        m_pAniso2xSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::Aniso2xSampler"));

        samplerDesc.MaxAnisotropy = 4.0f;
        m_pAniso4xSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::Aniso4xSampler"));

        samplerDesc.MaxAnisotropy = 8.0f;
        m_pAniso8xSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::Aniso8xSampler"));

        samplerDesc.MaxAnisotropy = 16.0f;
        m_pAniso16xSampler.reset(m_pDevice->CreateSampler(samplerDesc, "RendererBase::Aniso16xSampler"));

        RHI::RHIGraphicsPipelineStateDesc copyPSODesc;
        copyPSODesc.VS = GetShader("Copy.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        copyPSODesc.PS = GetShader("Copy.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        copyPSODesc.DepthStencilState.bDepthWrite = false;
        copyPSODesc.RTFormats[0] = m_pSwapchain->GetDesc()->ColorFormat;
        copyPSODesc.DepthStencilFormat = RHI::ERHIFormat::D32F;
        m_pCopyColorPSO = GetPipelineState(copyPSODesc, "CopyColorPSO");

        copyPSODesc.PS = GetShader("Copy.hlsl", "PSMain", RHI::ERHIShaderType::PS, { "OUTPUT_DEPTH = 1"});
        copyPSODesc.DepthStencilState.bDepthWrite = true;
        copyPSODesc.DepthStencilState.bDepthTest = true;
        copyPSODesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::Always;
        m_pCopyColorDepthPSO = GetPipelineState(copyPSODesc, "CopyColorDepthPSO");

        RHI::RHIComputePipelineStateDesc computePSODesc;
        computePSODesc.CS = GetShader("Copy.hlsl", "CopyDepthCS", RHI::ERHIShaderType::CS);
        m_pCopyDepthPSO = GetPipelineState(computePSODesc, "CopyDepthPSO");
    }

    void RendererBase::OnWindowResize(void* wndHandle, uint32_t width, uint32_t height)
    {
        WaitGPU();
        
        if (m_pSwapchain->GetDesc()->WindowHandle == wndHandle)
        {
            m_pSwapchain->Resize(width, height);
            m_DisplayWidth = width;
            m_DisplayHeight = height;
            m_RenderWidth = m_DisplayWidth;
            m_RenderHeight = m_DisplayHeight;
        }
    }

    void RendererBase::BeginFrame()
    {
        uint32_t frameIndex = m_pDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        m_pFrameFence->Wait(m_FrameFenceValue[frameIndex]);

        m_pDevice->BeginFrame();

        RHI::RHICommandList* pCmdList = m_pCmdList[frameIndex].get();
        pCmdList->ResetAllocator();
        pCmdList->Begin();

        RHI::RHICommandList* pComputeCmdList = m_pAsyncComputeCmdList[frameIndex].get();
        pComputeCmdList->ResetAllocator();
        pComputeCmdList->Begin();
    }

    void RendererBase::UploadResource()
    {
        if (m_PendingTextureUpload.empty() && m_PendingBufferUpload.empty()) return;

        uint32_t frameIndex = m_pDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHICommandList* pUploadeCmdList = m_pUploadCmdList[frameIndex].get();
        pUploadeCmdList->ResetAllocator();
        pUploadeCmdList->Begin();

        {
            GPU_EVENT_DEBUG(pUploadeCmdList, "RendererBase::UploadResources");
            
            for (size_t i = 0; i < m_PendingBufferUpload.size(); i++)
            {
                const BufferUpload& upload = m_PendingBufferUpload[i];
                pUploadeCmdList->CopyBuffer(upload.SBForUpload.Buffer, upload.Buffer, upload.SBForUpload.Offset, upload.Offset, upload.SBForUpload.Size);
            }

            for (size_t i = 0; i < m_PendingTextureUpload.size(); i++)
            {
                const TextureUpload& upload = m_PendingTextureUpload[i];
                pUploadeCmdList->CopyBufferToTexture(upload.SBForUpload.Buffer, upload.Texture, upload.MipLevel, upload.ArraySlice, upload.SBForUpload.Offset + upload.Offset);
            }
        }

        pUploadeCmdList->End();
        pUploadeCmdList->Signal(m_pUploadFence.get(), ++m_CurrentUploadFenceValue);
        pUploadeCmdList->Submit();

        RHI::RHICommandList* pCmdList = m_pCmdList[frameIndex].get();
        pCmdList->Wait(m_pUploadFence.get(), m_CurrentUploadFenceValue);

        if (m_pDevice->GetDesc().RenderBackend == RHI::ERHIRenderBackend::Vulkan)
        {
            for (size_t i = 0; i < m_PendingTextureUpload.size(); i++)
            {
                const TextureUpload& upload = m_PendingTextureUpload[i];
                pCmdList->TextureBarrier(upload.Texture, CalcSubresource(upload.Texture->GetDesc(), upload.MipLevel, upload.ArraySlice), RHI::RHIAccessCopyDst, RHI::RHIAccessMaskSRV);
            }
        }

        m_PendingBufferUpload.clear();
        m_PendingTextureUpload.clear();
    }

    void RendererBase::Render()
    {
        uint32_t frameIndex = m_pDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHICommandList* pCmdList = m_pCmdList[frameIndex].get();
        RHI::RHICommandList* pComputeCmdList = m_pAsyncComputeCmdList[frameIndex].get();

        GPU_EVENT_DEBUG(pCmdList, fmt::format("Render Frame {}", m_pDevice->GetFrameID()).c_str());

        m_pGPUDrivenDebugLine->Clear(pCmdList);

        SetupGlobalConstants(pCmdList);
        FlushComputePass(pCmdList);

        m_pRenderGraph->Execute(this, pCmdList, pComputeCmdList);
        
        RenderBackBufferPass(pCmdList);
    }

    void RendererBase::EndFrame()
    {
        uint32_t frameIndex = m_pDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHICommandList* pCmdList = m_pCmdList[frameIndex].get();
        RHI::RHICommandList* pComputeCmdList = m_pAsyncComputeCmdList[frameIndex].get();

        pComputeCmdList->End();
        pCmdList->End();

        m_FrameFenceValue[frameIndex] = ++m_CurrentFrameFenceValue;

        pCmdList->Present(m_pSwapchain.get());
        pCmdList->Signal(m_pFrameFence.get(), m_CurrentFrameFenceValue);
        pCmdList->Submit();

        m_pStagingBufferAllocators[frameIndex]->Reset();
        m_CBAllocator->Reset();
        m_pGPUScene->ResetFrameData();

        m_AnimationBatches.clear();
        m_IDPassBatches.clear();
        m_OutlinePassBatches.clear();

        m_pDevice->EndFrame();
    }

    void RendererBase::FlushComputePass(RHI::RHICommandList *pCmdList)
    {
        if (!m_AnimationBatches.empty())
        {
            GPU_EVENT_DEBUG(pCmdList, "Animation Pass");
            
            m_pGPUScene->BeginAnimationUpdate(pCmdList);
            for (size_t i = 0; i < m_AnimationBatches.size(); i++)
            {
                DispatchComputeBatch(pCmdList, m_AnimationBatches[i]);
            }
            m_pGPUScene->EndAnimationUpdate(pCmdList);
        }
    }

    void RendererBase::RenderBackBufferPass(RHI::RHICommandList *pCmdList)
    {
        m_pSwapchain->AcquireNextBackBuffer();
        pCmdList->TextureBarrier(m_pSwapchain->GetBackBuffer(), 0, RHI::RHIAccessPresent, RHI::RHIAccessRTV);

        RG::RGTexture* pColorRT = m_pRenderGraph->GetTexture(m_OutputColorHandle);
        RG::RGTexture* pDepthRT = m_pRenderGraph->GetTexture(m_OutputDepthHandle);

        {
            GPU_EVENT_DEBUG(pCmdList, "RenderBackBufferPass");

            m_pGPUDrivenDebugLine->PrepareForDraw(pCmdList);

            RHI::RHIRenderPassDesc renderPassDesc {};
            renderPassDesc.Color[0].Texture = m_pSwapchain->GetBackBuffer();
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
                m_pPointRepeatSampler->GetHeapIndex()
            };
            pCmdList->SetGraphicsConstants(0, constants, sizeof(constants));
            pCmdList->SetPipelineState(m_pCopyColorPSO);
            pCmdList->Draw(3);

            for (size_t i = 0; i < m_GUIBatches.size(); i++)
            {
                DrawBatch(pCmdList, m_GUIBatches[i]);
            }

            m_pGPUDrivenDebugLine->Draw(pCmdList);
            Core::VultanaEngine::GetEngineInstance()->GetEditor()->Render(pCmdList);

            pCmdList->EndRenderPass();
        }
        pCmdList->TextureBarrier(m_pSwapchain->GetBackBuffer(), 0, RHI::RHIAccessRTV, RHI::RHIAccessPresent);
    }

    void RendererBase::MouseHitTest()
    {
        if (m_bEnableObjectIDRendering)
        {
            WaitGPU();
            
            uint32_t x = m_MouseX;
            uint32_t y = m_MouseY;

            if (m_pObjectIDBuffer == nullptr) return;
            uint8_t* data = (uint8_t*)m_pObjectIDBuffer->GetCPUAddress();
            uint32_t dataOffset = m_ObjectIDRowPitch * y + x * sizeof(uint32_t);
            memcpy(&m_MouseHitObjectID, data + dataOffset, sizeof(uint32_t));

            m_bEnableObjectIDRendering = false;
        }
    }
} // namespace Vultana::Renderer
