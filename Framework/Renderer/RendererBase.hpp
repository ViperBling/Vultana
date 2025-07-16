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

        uint64_t GetFrameID() const { return m_pDevice->GetFrameID(); }
        class PipelineStateCache* GetPipelineStateCache() const { return m_pPipelineStateCache.get(); }
        class ShaderCompiler* GetShaderCompiler() const { return m_pShaderCompiler.get(); }
        class ShaderCache* GetShaderCache() const { return m_pShaderCache.get(); }
        uint32_t GetDisplayWidth() const { return m_DisplayWidth; }
        uint32_t GetDisplayHeight() const { return m_DisplayHeight; }
        uint32_t GetRenderWidth() const { return m_RenderWidth; }
        uint32_t GetRenderHeight() const { return m_RenderHeight; }

        RHI::RHIDevice* GetDevice() const { return m_pDevice.get(); }
        RHI::RHISwapchain* GetSwapchain() const { return m_pSwapchain.get(); }
        RHI::RHIShader* GetShader(const eastl::string& file, const eastl::string& entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string>& defines = {}, RHI::ERHIShaderCompileFlags flags = 0);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIGraphicsPipelineStateDesc& desc, const eastl::string& name);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIMeshShadingPipelineStateDesc& desc, const eastl::string& name);
        RHI::RHIPipelineState* GetPipelineState(const RHI::RHIComputePipelineStateDesc& desc, const eastl::string& name);
        void ReloadShaders();
        RHI::RHIDescriptor* GetPointSampler() const { return m_pPointRepeatSampler.get(); }
        RHI::RHIDescriptor* GetLinearSampler() const { return m_pBilinearRepeatSampler.get(); }
        RG::RenderGraph* GetRenderGraph() const { return m_pRenderGraph.get(); }

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
        uint32_t GetInstanceCount() const { return m_pGPUScene->GetInstanceCount(); }

        void UploadTexture(RHI::RHITexture* pTexture, const void* pData);
        void UploadBuffer(RHI::RHIBuffer* pBuffer, const void* pData, uint32_t offset, uint32_t dataSize);

        void SetupGlobalConstants(RHI::RHICommandList* pCmdList);

        LinearAllocator* GetConstantAllocator() { return m_CBAllocator.get(); }
        RenderBatch& AddBasePassBatch();
        ComputeBatch& AddAnimationBatch() { return m_AnimationBatches.emplace_back(*m_CBAllocator); }
        RenderBatch& AddOutlinePassBatch() { return m_OutlinePassBatches.emplace_back(*m_CBAllocator); }
        RenderBatch& AddObjectIDPassBatch() { return m_IDPassBatches.emplace_back(*m_CBAllocator); }
        RenderBatch& AddGUIBatch() { return m_GUIBatches.emplace_back(*m_CBAllocator); }

        void RequestMouseHitTest(uint32_t x, uint32_t y);
        bool IsEnableMouseHitTest() const { return m_bEnableObjectIDRendering; }
        uint32_t GetMouseHitObjectID() const { return m_MouseHitObjectID; }

        class ForwardBasePass* GetForwardBasePass() { return m_pForwardBasePass.get(); }

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
        eastl::unique_ptr<RHI::RHIDevice> m_pDevice;
        eastl::unique_ptr<RHI::RHISwapchain> m_pSwapchain;
        eastl::unique_ptr<class PipelineStateCache> m_pPipelineStateCache;
        eastl::unique_ptr<class ShaderCompiler> m_pShaderCompiler;
        eastl::unique_ptr<class ShaderCache> m_pShaderCache;
        eastl::unique_ptr<GPUScene> m_pGPUScene;
        eastl::unique_ptr<RG::RenderGraph> m_pRenderGraph;

        uint32_t m_DisplayWidth;
        uint32_t m_DisplayHeight;
        uint32_t m_RenderWidth;
        uint32_t m_RenderHeight;
        float m_UpscaleRatio = 1.0f;
        float m_MipBias = 0.0f;

        eastl::unique_ptr<LinearAllocator> m_CBAllocator;

        uint64_t m_CurrentFrameFenceValue = 0;
        uint64_t m_FrameFenceValue[RHI::RHI_MAX_INFLIGHT_FRAMES] = {};
        eastl::unique_ptr<RHI::RHIFence> m_pFrameFence;
        eastl::unique_ptr<RHI::RHICommandList> m_pCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t m_CurrentAsyncComputeFenceValue = 0;
        eastl::unique_ptr<RHI::RHIFence> m_pAsyncComputeFence;
        eastl::unique_ptr<RHI::RHICommandList> m_pAsyncComputeCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t m_CurrentUploadFenceValue = 0;
        eastl::unique_ptr<RHI::RHIFence> m_pUploadFence;
        eastl::unique_ptr<RHI::RHICommandList> m_pUploadCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];
        eastl::unique_ptr<StagingBufferAllocator> m_pStagingBufferAllocators[RHI::RHI_MAX_INFLIGHT_FRAMES];

        struct TextureUpload
        {
            RHI::RHITexture* Texture;
            uint32_t MipLevel;
            uint32_t ArraySlice;
            uint32_t Offset;
            StagingBuffer SBForUpload;
        };
        eastl::vector<TextureUpload> m_PendingTextureUpload;

        struct BufferUpload
        {
            RHI::RHIBuffer* Buffer;
            uint32_t Offset;
            StagingBuffer SBForUpload;
        };
        eastl::vector<BufferUpload> m_PendingBufferUpload;

        eastl::unique_ptr<RHI::RHIDescriptor> m_pAniso2xSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pAniso4xSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pAniso8xSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pAniso16xSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pPointRepeatSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pPointClampSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pBilinearRepeatSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pBilinearClampSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pTrilinearRepeatSampler;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pTrilinearClampSampler;

        eastl::unique_ptr<RenderResources::Texture2D> m_pPrevSceneDepthTexture;
        // eastl::unique_ptr<RenderResources::Texture2D> m_pPrevNormalTexture;
        eastl::unique_ptr<RenderResources::Texture2D> m_pPrevSceneColorTexture;
        RG::RGHandle m_PrevSceneDepthHandle;
        RG::RGHandle m_PrevNormalHandle;
        RG::RGHandle m_PrevSceneColorHandle;
        bool m_bHistoryValid = false;

        eastl::unique_ptr<RenderResources::TypedBuffer> m_pSPDCounterBuffer;

        bool m_bEnableObjectIDRendering = false;
        uint32_t m_MouseX = 0;
        uint32_t m_MouseY = 0;
        uint32_t m_MouseHitObjectID = UINT32_MAX;
        eastl::unique_ptr<RHI::RHIBuffer> m_pObjectIDBuffer;
        uint32_t m_ObjectIDRowPitch = 0;

        RG::RGHandle m_OutputColorHandle;
        RG::RGHandle m_OutputDepthHandle;
        
        RHI::RHIPipelineState* m_pCopyColorPSO = nullptr;
        RHI::RHIPipelineState* m_pCopyDepthPSO = nullptr;
        RHI::RHIPipelineState* m_pCopyColorDepthPSO = nullptr;

        eastl::unique_ptr<class ForwardBasePass> m_pForwardBasePass;

        eastl::unique_ptr<class GPUDrivenDebugLine> m_pGPUDrivenDebugLine;

        eastl::vector<ComputeBatch> m_AnimationBatches;

        eastl::vector<RenderBatch> m_OutlinePassBatches;
        eastl::vector<RenderBatch> m_IDPassBatches;
        eastl::vector<RenderBatch> m_GUIBatches;
    };
}