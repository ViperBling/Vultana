#pragma once

#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/RenderBatch.hpp"

namespace Renderer
{
    class FRendererBase;

    class FDeferredBasePass
    {
    public:
        FDeferredBasePass(FRendererBase* pRenderer);
        
        FRenderBatch& AddBatch();

        void Render1stPhase(RG::FRenderGraph* pRenderGraph);
        void Render2ndPhase(RG::FRenderGraph* pRenderGraph);

        RG::FRGHandle GetDiffuseRT() const { return m_DiffuseRT; }
        RG::FRGHandle GetNormalRT() const { return m_NormalRT; }
        RG::FRGHandle GetVelocityRT() const { return m_VelocityRT; }
        RG::FRGHandle GetDepthRT() const { return m_DepthRT; }

        RG::FRGHandle GetSecondPhaseMeshletListBuffer() const { return m_2ndPhaseMeshletListBuffer; }
        RG::FRGHandle GetSecondPhaseMeshletListCounterBuffer() const { return m_2ndPhaseMeshletListCounterBuffer; }

    private:
        void MergeBatches();

        void ResetCounter(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *firstPhaseMeshletCounter, RG::FRGBuffer *secondPhaseObjectCounter, RG::FRGBuffer *secondPhaseMeshletCounter);
        void InstanceCulling1stPhase(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *cullingResultUAV, RG::FRGBuffer *secondPhaseObjectListUAV, RG::FRGBuffer *secondPhaseObjectListCounterUAV);
        void InstanceCulling2ndPhase(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *pIndirectCommandBuffer, RG::FRGBuffer *cullingResultUAV, RG::FRGBuffer *objectListBufferSRV, RG::FRGBuffer *objectListCounterBufferSRV);

        void FlushBatches1stPhase(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *pIndirectCommandBuffer, RG::FRGBuffer *pMeshletListSRV, RG::FRGBuffer *pMeshletListCounterSRV);
        void FlushBatches2ndPhase(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *pIndirectCommandBuffer, RG::FRGBuffer *pMeshletListSRV, RG::FRGBuffer *pMeshletListCounterSRV);

        void BuildMeshletList(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *cullingResultSRV, RG::FRGBuffer *meshletListBufferUAV, RG::FRGBuffer *meshletListCounterBufferUAV);
        void BuildIndirectCommand(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *pCounterBufferSRV, RG::FRGBuffer *pCommandBufferUAV);

    private:
        FRendererBase* m_pRenderer;

        RHI::FRHIPipelineState* m_InstanceCulling1stPhasePSO = nullptr;
        RHI::FRHIPipelineState* m_InstanceCulling2ndPhasePSO = nullptr;

        RHI::FRHIPipelineState* m_BuildMeshletListPSO = nullptr;
        RHI::FRHIPipelineState* m_BuildInstanceCullingCmdPSO = nullptr;
        RHI::FRHIPipelineState* m_BuildIndirectCmdPSO = nullptr;

        eastl::vector<FRenderBatch> m_Instance;

        struct FIndirectBatch
        {
            RHI::FRHIPipelineState* PSO;
            uint32_t OriginMeshletListAddress;
            uint32_t OriginMeshletCount;
            uint32_t MeshletListBufferOffset;
        };
        eastl::vector<FIndirectBatch> m_IndirectBatches;
        eastl::vector<FRenderBatch> m_NonGPUDrivenBatches;

        uint32_t m_TotalInstanceCount = 0;
        uint32_t m_TotalMeshletCount = 0;
        uint32_t m_InstanceIndexAddress = 0;

        RG::FRGHandle m_DiffuseRT;
        RG::FRGHandle m_NormalRT;
        RG::FRGHandle m_VelocityRT;
        RG::FRGHandle m_DepthRT;
        
        RG::FRGHandle m_2ndPhaseObjectListBuffer;
        RG::FRGHandle m_2ndPhaseObjectListCounterBuffer;

        RG::FRGHandle m_2ndPhaseMeshletListBuffer;
        RG::FRGHandle m_2ndPhaseMeshletListCounterBuffer;
    };
}