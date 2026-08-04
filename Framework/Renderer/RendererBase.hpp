#pragma once

#include "RenderGraph/RenderGraph.hpp"
#include "RenderResources/IndexBuffer.hpp"
#include "RenderResources/RawBuffer.hpp"
#include "RenderResources/StructuredBuffer.hpp"
#include "RenderResources/Texture2D.hpp"
#include "RenderResources/TypedBuffer.hpp"
#include "RenderModules/GPUDrivenStats.hpp"
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
    class FGLFWindow;
}

namespace Renderer
{
    class FPipelineStateCache;
    class FShaderCompiler;
    class FShaderCache;
    class FHiZBuffer;
    class FGPUDrivenStats;

    class FRendererBase
    {
    public:
        FRendererBase();
        ~FRendererBase();

        bool CreateDevice(RHI::ERHIRenderBackend backend, void* windowHandle, uint32_t width, uint32_t height);
        virtual void RenderFrame();
        void WaitGPU();

        uint64_t GetFrameID() const { return m_pDevice->GetFrameID(); }
        class FPipelineStateCache* GetPipelineStateCache() const { return m_pPipelineStateCache.get(); }
        class FShaderCompiler* GetShaderCompiler() const { return m_pShaderCompiler.get(); }
        class FShaderCache* GetShaderCache() const { return m_pShaderCache.get(); }
        uint32_t GetDisplayWidth() const { return m_DisplayWidth; }
        uint32_t GetDisplayHeight() const { return m_DisplayHeight; }
        uint32_t GetRenderWidth() const { return m_RenderWidth; }
        uint32_t GetRenderHeight() const { return m_RenderHeight; }

        RHI::FRHIDevice* GetDevice() const { return m_pDevice.get(); }
        RHI::FRHISwapchain* GetSwapchain() const { return m_pSwapchain.get(); }
        RHI::FRHIShader* GetShader(const eastl::string& file, const eastl::string& entryPoint, RHI::ERHIShaderType type, const eastl::vector<eastl::string>& defines = {}, RHI::ERHIShaderCompileFlags flags = 0);
        RHI::FRHIPipelineState* GetPipelineState(const RHI::FRHIGraphicsPipelineStateDesc& desc, const eastl::string& name);
        RHI::FRHIPipelineState* GetPipelineState(const RHI::FRHIMeshShadingPipelineStateDesc& desc, const eastl::string& name);
        RHI::FRHIPipelineState* GetPipelineState(const RHI::FRHIComputePipelineStateDesc& desc, const eastl::string& name);
        void ReloadShaders();
        RHI::FRHIDescriptor* GetPointSampler() const { return m_pPointRepeatSampler.get(); }
        RHI::FRHIDescriptor* GetLinearSampler() const { return m_pBilinearRepeatSampler.get(); }
        RG::FRenderGraph* GetRenderGraph() const { return m_pRenderGraph.get(); }

        RenderResources::FTexture2D* CreateTexture2D(const eastl::string& file, bool srgb);
        RenderResources::FTexture2D* CreateTexture2D(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags, const eastl::string& name);

        RenderResources::FIndexBuffer* CreateIndexBuffer(const void* data, uint32_t stride, uint32_t indexCount, const eastl::string& name, RHI::ERHIMemoryType memoryType = RHI::ERHIMemoryType::GPUOnly);
        RenderResources::FStructuredBuffer* CreateStructuredBuffer(const void* data, uint32_t stride, uint32_t elementCount, const eastl::string& name, RHI::ERHIMemoryType memoryType = RHI::ERHIMemoryType::GPUOnly, bool isUAV = false);
        RenderResources::FRawBuffer* CreateRawBuffer(const void* data, uint32_t size, const eastl::string& name, RHI::ERHIMemoryType memoryType = RHI::ERHIMemoryType::GPUOnly, bool isUAV = false);

        RHI::FRHIBuffer* GetSceneStaticBuffer() const;
        OffsetAllocator::Allocation AllocateSceneStaticBuffer(const void* data, uint32_t size);
        void FreeSceneStaticBuffer(OffsetAllocator::Allocation allocation);

        RHI::FRHIBuffer* GetSceneAnimationBuffer() const;
        OffsetAllocator::Allocation AllocateSceneAnimationBuffer(uint32_t size);
        void FreeSceneAnimationBuffer(OffsetAllocator::Allocation allocation);
        
        uint32_t AllocateSceneConstantBuffer(const void* data, uint32_t size);
        uint32_t AddInstance(const FInstanceData& instanceData);
        uint32_t GetInstanceCount() const { return m_pGPUScene->GetInstanceCount(); }

        void UploadTexture(RHI::FRHITexture* pTexture, const void* pData);
        void UploadBuffer(RHI::FRHIBuffer* pBuffer, const void* pData, uint32_t offset, uint32_t dataSize);

        void SetupGlobalConstants(RHI::FRHICommandList* pCmdList);

        FLinearAllocator* GetConstantAllocator() { return m_CBAllocator.get(); }
        FRenderBatch& AddBasePassBatch();
        FComputeBatch& AddAnimationBatch() { return m_AnimationBatches.emplace_back(*m_CBAllocator); }
        FRenderBatch& AddOutlinePassBatch() { return m_OutlinePassBatches.emplace_back(*m_CBAllocator); }
        FRenderBatch& AddObjectIDPassBatch() { return m_IDPassBatches.emplace_back(*m_CBAllocator); }
        FRenderBatch& AddGUIBatch() { return m_GUIBatches.emplace_back(*m_CBAllocator); }

        void RequestMouseHitTest(uint32_t x, uint32_t y);
        bool IsEnableMouseHitTest() const { return m_bEnableObjectIDRendering; }
        uint32_t GetMouseHitObjectID() const { return m_MouseHitObjectID; }

        class FDeferredBasePass* GetDeferredBasePass() { return m_pDeferredBasePass.get(); }
        class FHiZBuffer* GetHiZBuffer() { return m_pHZB.get(); }
        class FGPUDrivenStats* GetGPUDrivenStats() { return m_pGPUDrivenStats.get(); }
        RenderResources::FTypedBuffer* GetSPDCounterBuffer() { return m_pSPDCounterBuffer.get(); }
        RG::FRGHandle GetPrevSceneDepthHandle() const { return m_PrevSceneDepthHandle; }
        bool IsGPUDrivenStatsEnabled() const { return m_bGPUDrivenStatsEnabled; }
        void SetGPUDrivenStatsEnabled(bool enabled) { m_bGPUDrivenStatsEnabled = enabled; }
        bool IsShowMeshletsEnabled() const { return m_bShowMeshlets; }
        void SetShowMeshletsEnabled(bool enabled) { m_bShowMeshlets = enabled; }

    protected:
        virtual void CreateCommonResources();
        void OnWindowResize(void* wndHandle, uint32_t width, uint32_t height);

        virtual void BeginFrame();
        virtual void UploadResource();
        virtual void Render();
        virtual void EndFrame();

        void ObjectIDPass(RG::FRGHandle& depth);
        void OutlinePass(RG::FRGHandle& color, RG::FRGHandle& depth);
        void CopyHistoryPass(RG::FRGHandle sceneDepth, /* RG::RGHandle sceneNormal, */ RG::FRGHandle sceneColor);

        void FlushComputePass(RHI::FRHICommandList* pCmdList);
        void ImportPrevFrameTextures();
        virtual void RenderBackBufferPass(RHI::FRHICommandList* pCmdList);
    
    private:
        void BuildRenderGraph(RG::FRGHandle& outputColor, RG::FRGHandle& outputDepth);

        void MouseHitTest();

    private:
        eastl::unique_ptr<RHI::FRHIDevice> m_pDevice;
        eastl::unique_ptr<RHI::FRHISwapchain> m_pSwapchain;
        eastl::unique_ptr<class FPipelineStateCache> m_pPipelineStateCache;
        eastl::unique_ptr<class FShaderCompiler> m_pShaderCompiler;
        eastl::unique_ptr<class FShaderCache> m_pShaderCache;
        eastl::unique_ptr<FGPUScene> m_pGPUScene;
        eastl::unique_ptr<RG::FRenderGraph> m_pRenderGraph;

        uint32_t m_DisplayWidth;
        uint32_t m_DisplayHeight;
        uint32_t m_RenderWidth;
        uint32_t m_RenderHeight;
        float m_UpscaleRatio = 1.0f;
        float m_MipBias = 0.0f;

        eastl::unique_ptr<FLinearAllocator> m_CBAllocator;

        uint64_t m_CurrentFrameFenceValue = 0;
        uint64_t m_FrameFenceValue[RHI::RHI_MAX_INFLIGHT_FRAMES] = {};
        eastl::unique_ptr<RHI::FRHIFence> m_pFrameFence;
        eastl::unique_ptr<RHI::FRHICommandList> m_pCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t m_CurrentAsyncComputeFenceValue = 0;
        eastl::unique_ptr<RHI::FRHIFence> m_pAsyncComputeFence;
        eastl::unique_ptr<RHI::FRHICommandList> m_pAsyncComputeCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];

        uint64_t m_CurrentUploadFenceValue = 0;
        eastl::unique_ptr<RHI::FRHIFence> m_pUploadFence;
        eastl::unique_ptr<RHI::FRHICommandList> m_pUploadCmdList[RHI::RHI_MAX_INFLIGHT_FRAMES];
        eastl::unique_ptr<FStagingBufferAllocator> m_pStagingBufferAllocators[RHI::RHI_MAX_INFLIGHT_FRAMES];

        struct FTextureUpload
        {
            RHI::FRHITexture* Texture;
            uint32_t MipLevel;
            uint32_t ArraySlice;
            uint32_t Offset;
            FStagingBuffer SBForUpload;
        };
        eastl::vector<FTextureUpload> m_PendingTextureUpload;

        struct FBufferUpload
        {
            RHI::FRHIBuffer* Buffer;
            uint32_t Offset;
            FStagingBuffer SBForUpload;
        };
        eastl::vector<FBufferUpload> m_PendingBufferUpload;

        eastl::unique_ptr<RHI::FRHIDescriptor> m_pAniso2xSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pAniso4xSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pAniso8xSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pAniso16xSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pPointRepeatSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pPointClampSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pBilinearRepeatSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pBilinearClampSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pTrilinearRepeatSampler;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pTrilinearClampSampler;

        eastl::unique_ptr<RenderResources::FTexture2D> m_pPrevSceneDepthTexture;
        // eastl::unique_ptr<RenderResources::Texture2D> m_pPrevNormalTexture;
        eastl::unique_ptr<RenderResources::FTexture2D> m_pPrevSceneColorTexture;
        RG::FRGHandle m_PrevSceneDepthHandle;
        RG::FRGHandle m_PrevNormalHandle;
        RG::FRGHandle m_PrevSceneColorHandle;
        bool m_bHistoryValid = false;

        eastl::unique_ptr<RenderResources::FTypedBuffer> m_pSPDCounterBuffer;

        bool m_bEnableObjectIDRendering = false;
        uint32_t m_MouseX = 0;
        uint32_t m_MouseY = 0;
        uint32_t m_MouseHitObjectID = UINT32_MAX;
        eastl::unique_ptr<RHI::FRHIBuffer> m_pObjectIDBuffer;
        uint32_t m_ObjectIDRowPitch = 0;

        RG::FRGHandle m_OutputColorHandle;
        RG::FRGHandle m_OutputDepthHandle;
        
        RHI::FRHIPipelineState* m_pCopyColorPSO = nullptr;
        RHI::FRHIPipelineState* m_pCopyDepthPSO = nullptr;
        RHI::FRHIPipelineState* m_pCopyColorDepthPSO = nullptr;

        eastl::unique_ptr<class FDeferredBasePass> m_pDeferredBasePass;

        eastl::unique_ptr<class FGPUDrivenDebugLine> m_pGPUDrivenDebugLine;
        eastl::unique_ptr<class FHiZBuffer> m_pHZB;
        eastl::unique_ptr<class FGPUDrivenStats> m_pGPUDrivenStats;
        bool m_bGPUDrivenStatsEnabled = false;
        bool m_bShowMeshlets = false;

        // Per-frame transient handles, cached in BuildRenderGraph and resolved in SetupGlobalConstants
        RG::FRGHandle m_CullingHZB1stPhaseHandle;
        RG::FRGHandle m_CullingHZB2ndPhaseHandle;
        RG::FRGHandle m_SceneHZBHandle;
        RG::FRGHandle m_SecondPhaseMeshletListHandle;
        RG::FRGHandle m_SecondPhaseMeshletListCounterHandle;

        eastl::vector<FComputeBatch> m_AnimationBatches;

        eastl::vector<FRenderBatch> m_OutlinePassBatches;
        eastl::vector<FRenderBatch> m_IDPassBatches;
        eastl::vector<FRenderBatch> m_GUIBatches;
    };
}