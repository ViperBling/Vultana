#pragma once

#include "RenderGraph/RenderGraph.hpp"
#include "RenderResources/IndexBuffer.hpp"
#include "RenderResources/RawBuffer.hpp"
#include "RenderResources/StructuredBuffer.hpp"
#include "RenderResources/Texture2D.hpp"
#include "RenderResources/TypedBuffer.hpp"
#include "GPUScene.hpp"
#include "RenderBatch.hpp"
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
        virtual void RenderFrame();
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
        RHI::RHIShader* GetShader(const eastl::string& file, const eastl::string& entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string>& defines = {}, RHI::ERHIShaderCompileFlags flags = 0);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc& desc, const eastl::string& name);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIMeshShadingPipelineStateDesc& desc, const eastl::string& name);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIComputePipelineStateDesc& desc, const eastl::string& name);
        void ReloadShaders();
        RHI::RHIDescriptor* GetPointSampler() const { return mpPointRepeatSampler.get(); }
        RHI::RHIDescriptor* GetLinearSampler() const { return mpBilinearRepeatSampler.get(); }
        RG::RenderGraph* GetRenderGraph() const { return mpRenderGraph.get(); }

        RenderResources::Texture2D* CreateTexture2D(const eastl::string& file, bool srgb);
        RenderResources::Texture2D* CreateTexture2D(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags, const eastl::string& name);

        RenderResources::IndexBuffer* CreateIndexBuffer(const void* data, uint32_t stride, uint32_t indexCount, const eastl::string& name, RHI::ERHIMemoryType memoryType = RHI::ERHIMemoryType::GPUOnly);
        RenderResources::StructuredBuffer* CreateStructuredBuffer(const void* data, uint32_t stride, uint32_t elementCount, const eastl::string& name, RHI::ERHIMemoryType memoryType = RHI::ERHIMemoryType::GPUOnly, bool isUAV = false);
        RenderResources::RawBuffer* CreateRawBuffer(const void* data, uint32_t size, const eastl::string& name, RHI::ERHIMemoryType memoryType = RHI::ERHIMemoryType::GPUOnly, bool isUAV = false);

        RHI::RHIBuffer* GetSceneStaticBuffer() const;
        OffsetAllocator::Allocation AllocateSceneStaticBuffer(const void* data, uint32_t size);
        void FreeSceneStaticBuffer(OffsetAllocator::Allocation allocation);

        RHI::RHIBuffer* GetSceneAnimationBuffer() const;
        OffsetAllocator::Allocation AllocateSceneAnimationBuffer(uint32_t size);
        void FreeSceneAnimationBuffer(OffsetAllocator::Allocation allocation);
        
        uint32_t AllocateSceneConstantBuffer(const void* data, uint32_t size);
        uint32_t AddInstance(const FInstanceData& instanceData);
        uint32_t GetInstanceCount() const { return mpGPUScene->GetInstanceCount(); }

        void UploadTexture(RHI::RHITexture* pTexture, const void* pData);
        void UploadBuffer(RHI::RHIBuffer* pBuffer, const void* pData, uint32_t offset, uint32_t dataSize);

        void SetupGlobalConstants(RHI::RHICommandList* pCmdList);

        LinearAllocator* GetConstantAllocator() { return mCBAllocator.get(); }
        RenderBatch& AddBasePassBatch();
        ComputeBatch& AddAnimationBatch() { return mAnimationBatches.emplace_back(*mCBAllocator); }
        RenderBatch& AddOutlinePassBatch() { return mOutlinePassBatches.emplace_back(*mCBAllocator); }
        RenderBatch& AddObjectIDPassBatch() { return mIDPassBatches.emplace_back(*mCBAllocator); }
        RenderBatch& AddGUIBatch() { return mGUIBatches.emplace_back(*mCBAllocator); }

        void RequestMouseHitTest(uint32_t x, uint32_t y);
        bool IsEnableMouseHitTest() const { return mbEnableObjectIDRendering; }
        uint32_t GetMouseHitObjectID() const { return mMouseHitObjectID; }

        class ForwardBasePass* GetForwardBasePass() { return mpForwardBasePass.get(); }

    protected:
        virtual void CreateCommonResources();
        void OnWindowResize(void* wndHandle, uint32_t width, uint32_t height);

        virtual void BeginFrame();
        virtual void UploadResource();
        virtual void Render();
        virtual void EndFrame();

        void ObjectIDPass(RG::RGHandle& depth);
        void OutlinePass(RG::RGHandle& color, RG::RGHandle& depth);
        void CopyHistoryPass(RG::RGHandle sceneDepth, /* RG::RGHandle sceneNormal, */ RG::RGHandle sceneColor);

        void FlushComputePass(RHI::RHICommandList* pCmdList);
        void ImportPrevFrameTextures();
        virtual void RenderBackBufferPass(RHI::RHICommandList* pCmdList);
    
    private:
        void BuildRenderGraph(RG::RGHandle& outputColor, RG::RGHandle& outputDepth);

        void MouseHitTest();

    private:
        eastl::unique_ptr<RHI::RHIDevice> mpDevice;
        eastl::unique_ptr<RHI::RHISwapchain> mpSwapchain;
        eastl::unique_ptr<class PipelineStateCache> mpPipelineStateCache;
        eastl::unique_ptr<class ShaderCompiler> mpShaderCompiler;
        eastl::unique_ptr<class ShaderCache> mpShaderCache;
        eastl::unique_ptr<GPUScene> mpGPUScene;
        eastl::unique_ptr<RG::RenderGraph> mpRenderGraph;

        uint32_t mDisplayWidth;
        uint32_t mDisplayHeight;
        uint32_t mRenderWidth;
        uint32_t mRenderHeight;
        float mUpscaleRatio = 1.0f;
        float mMipBias = 0.0f;

        eastl::unique_ptr<LinearAllocator> mCBAllocator;

        uint64_t mCurrentFrameFenceValue = 0;
        uint64_t mFrameFenceValue[RHI::RHI_MAX_INFLIGHT_FRAMES] = {};
        eastl::unique_ptr<RHI::RHIFence> mpFrameFence;
        eastl::unique_ptr<RHI::RHICommandList> mpCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t mCurrentAsyncComputeFenceValue = 0;
        eastl::unique_ptr<RHI::RHIFence> mpAsyncComputeFence;
        eastl::unique_ptr<RHI::RHICommandList> mpAsyncComputeCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t mCurrentUploadFenceValue = 0;
        eastl::unique_ptr<RHI::RHIFence> mpUploadFence;
        eastl::unique_ptr<RHI::RHICommandList> mpUploadCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];
        eastl::unique_ptr<StagingBufferAllocator> mpStagingBufferAllocators[RHI::RHI_MAX_INFLIGHT_FRAMES];

        struct TextureUpload
        {
            RHI::RHITexture* Texture;
            uint32_t MipLevel;
            uint32_t ArraySlice;
            uint32_t Offset;
            StagingBuffer SBForUpload;
        };
        eastl::vector<TextureUpload> mPendingTextureUpload;

        struct BufferUpload
        {
            RHI::RHIBuffer* Buffer;
            uint32_t Offset;
            StagingBuffer SBForUpload;
        };
        eastl::vector<BufferUpload> mPendingBufferUpload;

        eastl::unique_ptr<RHI::RHIDescriptor> mpAniso2xSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpAniso4xSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpAniso8xSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpAniso16xSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpPointRepeatSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpPointClampSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpBilinearRepeatSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpBilinearClampSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpTrilinearRepeatSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> mpTrilinearClampSampler;

        eastl::unique_ptr<RenderResources::Texture2D> mpPrevSceneDepthTexture;
        // eastl::unique_ptr<RenderResources::Texture2D> mpPrevNormalTexture;
        eastl::unique_ptr<RenderResources::Texture2D> mpPrevSceneColorTexture;
        RG::RGHandle m_PrevSceneDepthHandle;
        RG::RGHandle m_PrevNormalHandle;
        RG::RGHandle m_PrevSceneColorHandle;
        bool mbHistoryValid = false;

        eastl::unique_ptr<RenderResources::TypedBuffer> mpSPDCounterBuffer;

        bool mbEnableObjectIDRendering = false;
        uint32_t mMouseX = 0;
        uint32_t mMouseY = 0;
        uint32_t mMouseHitObjectID = UINT32_MAX;
        eastl::unique_ptr<RHI::RHIBuffer> mpObjectIDBuffer;
        uint32_t mObjectIDRowPitch = 0;

        RG::RGHandle mOutputColorHandle;
        RG::RGHandle mOutputDepthHandle;
        
        RHI::RHIPipelineState* mpCopyColorPSO = nullptr;
        RHI::RHIPipelineState* mpCopyDepthPSO = nullptr;
        RHI::RHIPipelineState* mpCopyColorDepthPSO = nullptr;

        eastl::unique_ptr<class ForwardBasePass> mpForwardBasePass;

        eastl::unique_ptr<class GPUDrivenDebugLine> mpGPUDrivenDebugLine;

        eastl::vector<ComputeBatch> mAnimationBatches;

        eastl::vector<RenderBatch> mOutlinePassBatches;
        eastl::vector<RenderBatch> mIDPassBatches;
        eastl::vector<RenderBatch> mGUIBatches;
    };
}