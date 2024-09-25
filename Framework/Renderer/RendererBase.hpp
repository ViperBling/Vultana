#pragma once

#include "RenderResources/IndexBuffer.hpp"
#include "RenderResources/StructuredBuffer.hpp"
#include "RenderResources/Texture2D.hpp"
#include "StagingBufferAllocator.hpp"

#include "RHI/RHI.hpp"
#include "Utilities/Utility.hpp"
#include "Utilities/Math.hpp"

#include <iostream>
#include <deque>
#include <memory>
#include <functional>

namespace Window
{
    class GLFWindow;
}

namespace Renderer
{
    class PipelineStateCache;
    class ShaderCompiler;
    class ShaderCache;

    class RendererBase
    {
    public:
        RendererBase();
        ~RendererBase();

        bool CreateDevice(RHI::ERHIRenderBackend backend, void* windowHandle, uint32_t width, uint32_t height);
        void RenderFrame();
        void WaitGPU();

        uint64_t GetFrameID() const { return mpDevice->GetFrameID(); }
        class PipelineStateCache* GetPipelineStateCache() const { return mpPipelineStateCache.get(); }
        class ShaderCompiler* GetShaderCompiler() const { return mpShaderCompiler.get(); }
        class ShaderCache* GetShaderCache() const { return mpShaderCache.get(); }
        uint32_t GetDisplayWidth() const { return mDisplayWidth; }
        uint32_t GetDisplayHeight() const { return mDisplayHeight; }
        uint32_t GetRenderWidth() const { return mRenderWidth; }
        uint32_t GetRenderHeight() const { return mRenderHeight; }

        RHI::RHIDevice* GetDevice() const { return mpDevice.get(); }
        RHI::RHISwapchain* GetSwapchain() const { return mpSwapchain.get(); }
        RHI::RHIShader* GetShader(const std::string& file, const std::string& entryPoint, RHI::ERHIShaderType type, const std::vector<std::string>& defines = {}, RHI::ERHIShaderCompileFlags flags = 0);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc& desc, const std::string& name);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIComputePipelineStateDesc& desc, const std::string& name);
        void ReloadShaders();

        Texture2D* CreateTexture2D(const std::string& file, bool srgb);
        Texture2D* CreateTexture2D(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags, const std::string& name);

        IndexBuffer* CreateIndexBuffer(const void* data, uint32_t stride, uint32_t indexCount, const std::string& name, RHI::ERHIMemoryType memoryType = RHI::ERHIMemoryType::GPUOnly);
        StructuredBuffer* CreateStructuredBuffer(const void* data, uint32_t stride, uint32_t elementCount, const std::string& name, RHI::ERHIMemoryType memoryType = RHI::ERHIMemoryType::GPUOnly, bool isUAV = false);

        void UploadBuffer(RHI::RHIBuffer* pBuffer, const void* pData, uint32_t offset, uint32_t dataSize);


    private:
        void CreateCommonResources();
        void OnWindowResize(Window::GLFWindow& wndHandle, uint32_t width, uint32_t height);

        void BeginFrame();
        void UploadResource();
        void Render();
        void EndFrame();

        void RenderBackBufferPass(RHI::RHICommandList* pCmdList);
        
    private:
        std::unique_ptr<RHI::RHIDevice> mpDevice;
        std::unique_ptr<RHI::RHISwapchain> mpSwapchain;
        std::unique_ptr<class PipelineStateCache> mpPipelineStateCache;
        std::unique_ptr<class ShaderCompiler> mpShaderCompiler;
        std::unique_ptr<class ShaderCache> mpShaderCache;

        std::unique_ptr<RHI::RHIDescriptor> mpPointSampler;
        std::unique_ptr<RHI::RHIDescriptor> mpLinearSampler;

        uint32_t mDisplayWidth;
        uint32_t mDisplayHeight;
        uint32_t mRenderWidth;
        uint32_t mRenderHeight;
        float mUpscaleRatio = 1.0f;
        float mMipBias = 0.0f;

        uint64_t mCurrentFrameFenceValue = 0;
        uint64_t mFrameFenceValue[RHI::RHI_MAX_INFLIGHT_FRAMES] = {};
        std::unique_ptr<RHI::RHIFence> mpFrameFence;
        std::unique_ptr<RHI::RHICommandList> mpCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t mCurrentAsyncComputeFenceValue = 0;
        std::unique_ptr<RHI::RHIFence> mpAsyncComputeFence;
        std::unique_ptr<RHI::RHICommandList> mpAsyncComputeCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t mCurrentUploadFenceValue = 0;
        std::unique_ptr<RHI::RHIFence> mpUploadFence;
        std::unique_ptr<RHI::RHICommandList> mpUploadCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];
        std::unique_ptr<StagingBufferAllocator> mpStagingBufferAllocators[RHI::RHI_MAX_INFLIGHT_FRAMES];

        struct BufferUpload
        {
            RHI::RHIBuffer* Buffer;
            uint32_t Offset;
            StagingBuffer SBForUpload;
        };
        std::vector<BufferUpload> mPendingBufferUpload;

        // For Test
        std::unique_ptr<Texture2D> mpTestRT;
        std::unique_ptr<Texture2D> mpTestDepthRT;
        
        std::unique_ptr<StructuredBuffer> mTestVertexBuffer = nullptr;
        std::unique_ptr<IndexBuffer> mTestIndexBuffer = nullptr;
        RHI::RHIPipelineState* mTestPSO = nullptr;
        RHI::RHIPipelineState* mpCopyColorPSO = nullptr;
        RHI::RHIPipelineState* mpCopyDepthPSO = nullptr;
        RHI::RHIPipelineState* mpCopyColorDepthPSO = nullptr;
    };
}