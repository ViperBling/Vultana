#include "RendererBase.hpp"
#include "PipelineStateCache.hpp"
#include "ShaderCompiler.hpp"
#include "ShaderCache.hpp"
#include "RHI/RHI.hpp"
#include "Core/VultanaEngine.hpp"
#include "Utilities/Log.hpp"
#include "Windows/GLFWindow.hpp"

#include <optional>
#include <algorithm>
#include <fstream>

namespace Renderer
{
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
        }

        // CreateCommonResources();

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
    }

    void RendererBase::Render()
    {
        uint32_t frameIndex = mpDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHICommandList* pCmdList = mpCmdList[frameIndex].get();
        RHI::RHICommandList* pComputeCmdList = mpAsyncComputeCmdList[frameIndex].get();
        
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
        mpSwapchain->AcquireNextBackBuffer();
        pCmdList->TextureBarrier(mpSwapchain->GetBackBuffer(), 0, RHI::RHIAccessPresent, RHI::RHIAccessRTV);

        pCmdList->TextureBarrier(mpSwapchain->GetBackBuffer(), 0, RHI::RHIAccessRTV, RHI::RHIAccessPresent);
    }
} // namespace Vultana::Renderer
