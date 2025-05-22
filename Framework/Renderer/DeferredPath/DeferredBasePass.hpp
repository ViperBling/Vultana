#pragma once

#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/RenderBatch.hpp"

namespace Renderer
{
    class RendererBase;

    class DeferredBasePass
    {
    public:
        DeferredBasePass(RendererBase* pRenderer);

        RenderBatch& AddBatch();
        void Render1stPhase(RG::RenderGraph* pRG);
        void Render2ndPhase(RG::RenderGraph* pRG);

        RG::RGHandle GetDiffuseRT() const { return mDiffuseRT; }
        RG::RGHandle GetSpecularRT() const { return mSpecularRT; }
        RG::RGHandle GetNormalRT() const { return mNormalRT; }
        RG::RGHandle GetEmissiveRT() const { return mEmissiveRT; }
        RG::RGHandle GetCustomDataRT() const { return mCustomDataRT; }
        RG::RGHandle GetDepthRT() const { return mDepthRT; }

        RG::RGHandle GetMeshletListBuffer2ndPhase() const { return mMeshletListBuffer2ndPhase; }
        RG::RGHandle GetMeshletListCounterBuffer2ndPhase() const { return mMeshletListCounterBuffer2ndPhase; }

    private:
        void MergeBatches();
        void ResetCounter(RHI::RHICommandList* pCmdList, RG::RGBuffer* meshletCounter1stPhase, RG::RGBuffer* objectCounter2ndPhase, RG::RGBuffer* meshletCounter2ndPhase);

        void InstanceCulling1stPhase(RHI::RHICommandList* pCmdList, RG::RGBuffer* cullingResultUAV, RG::RGBuffer* ObjectListUAV2ndPhase, RG::RGBuffer* ObjectListCounterUAV2ndPhase);
        void InstanceCulling2ndPhase(RHI::RHICommandList* pCmdList, RG::RGBuffer* pIndirectCmdBuffer, RG::RGBuffer* cullingResultUAV, RG::RGBuffer* objectListBufferSRV, RG::RGBuffer* objectListCounterBufferSRV);

        void FlushBatches1stPhase(RHI::RHICommandList* pCmdList, RG::RGBuffer* pIndirectCmdBuffer, RG::RGBuffer* pMeshletListSRV, RG::RGBuffer* pMeshletListCounterSRV);
        void FlushBatches2ndPhase(RHI::RHICommandList* pCmdList, RG::RGBuffer* pIndirectCmdBuffer, RG::RGBuffer* pMeshletListSRV, RG::RGBuffer* pMeshletListCounterSRV);

        void BuildMeshletList(RHI::RHICommandList* pCmdList, RG::RGBuffer* cullingResultSRV, RG::RGBuffer* meshletListBufferUAV, RG::RGBuffer* meshletListCounterBufferUAV);
        void BuildIndirectCommand(RHI::RHICommandList* pCmdList, RG::RGBuffer* pCounterBufferSRV, RG::RGBuffer* pIndirectCmdBufferUAV);

    private:
        RendererBase* mpRenderer;

        RHI::RHIPipelineState* mInstanceCulling1stPhasePSO = nullptr;
        RHI::RHIPipelineState* mInstanceCulling2ndPhasePSO = nullptr;

        RHI::RHIPipelineState* mBuildMeshletListPSO = nullptr;
        RHI::RHIPipelineState* mBuildInstanceCullingCmdPSO = nullptr;
        RHI::RHIPipelineState* mBuildIndirectCmdPSO = nullptr;

        eastl::vector<RenderBatch> mInstance;

        struct IndirectBatch
        {
            RHI::RHIPipelineState* PSO;
            uint32_t OriginMeshletListAddress;
            uint32_t OriginMeshletCount;
            uint32_t MeshletListBufferOffset;
        };
        eastl::vector<IndirectBatch> mIndirectBatches;
        eastl::vector<RenderBatch> mNonGPUDrivenBatches;

        uint32_t mTotalInstanceCount = 0;
        uint32_t mTotalMeshletCount = 0;

        uint32_t mInstnaceIndexAddress = 0;

        RG::RGHandle mDiffuseRT;
        RG::RGHandle mSpecularRT;
        RG::RGHandle mNormalRT;
        RG::RGHandle mEmissiveRT;
        RG::RGHandle mCustomDataRT;
        RG::RGHandle mDepthRT;

        RG::RGHandle mObjectListBuffer2ndPhase;
        RG::RGHandle mObjectListCounterBuffer2ndPhase;

        RG::RGHandle mMeshletListBuffer2ndPhase;
        RG::RGHandle mMeshletListCounterBuffer2ndPhase;
    };
}