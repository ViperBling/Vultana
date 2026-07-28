#pragma once

#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/RenderBatch.hpp"

namespace Renderer
{
    class RendererBase;

    class ForwardBasePass
    {
    public:
        ForwardBasePass(RendererBase* pRenderer);
        
        RenderBatch& AddBatch();

        void Render1stPhase(RG::RenderGraph* pRenderGraph);
        void Render2ndPhase(RG::RenderGraph* pRenderGraph);

        RG::RGHandle GetDiffuseRT() const { return m_DiffuseRT; }
        RG::RGHandle GetNormalRT() const { return m_NormalRT; }
        RG::RGHandle GetVelocityRT() const { return m_VelocityRT; }
        RG::RGHandle GetDepthRT() const { return m_DepthRT; }

        RG::RGHandle GetSecondPhaseMeshletListBuffer() const { return m_2ndPhaseMeshletListBuffer; }
        RG::RGHandle GetSecondPhaseMeshletListCounterBuffer() const { return m_2ndPhaseMeshletListCounterBuffer; }

    private:
        void MergeBatches();

        void ResetCounter(RHI::RHICommandList *pCmdList, RG::RGBuffer *firstPhaseMeshletCounter, RG::RGBuffer *secondPhaseObjectCounter, RG::RGBuffer *secondPhaseMeshletCounter);
        void InstanceCulling1stPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *cullingResultUAV, RG::RGBuffer *secondPhaseObjectListUAV, RG::RGBuffer *secondPhaseObjectListCounterUAV);
        void InstanceCulling2ndPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCommandBuffer, RG::RGBuffer *cullingResultUAV, RG::RGBuffer *objectListBufferSRV, RG::RGBuffer *objectListCounterBufferSRV);

        void FlushBatches1stPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCommandBuffer, RG::RGBuffer *pMeshletListSRV, RG::RGBuffer *pMeshletListCounterSRV);
        void FlushBatches2ndPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCommandBuffer, RG::RGBuffer *pMeshletListSRV, RG::RGBuffer *pMeshletListCounterSRV);

        void BuildMeshletList(RHI::RHICommandList *pCmdList, RG::RGBuffer *cullingResultSRV, RG::RGBuffer *meshletListBufferUAV, RG::RGBuffer *meshletListCounterBufferUAV);
        void BuildIndirectCommand(RHI::RHICommandList *pCmdList, RG::RGBuffer *pCounterBufferSRV, RG::RGBuffer *pCommandBufferUAV);

    private:
        RendererBase* m_pRenderer;

        RHI::RHIPipelineState* m_InstanceCulling1stPhasePSO = nullptr;
        RHI::RHIPipelineState* m_InstanceCulling2ndPhasePSO = nullptr;

        RHI::RHIPipelineState* m_BuildMeshletListPSO = nullptr;
        RHI::RHIPipelineState* m_BuildInstanceCullingCmdPSO = nullptr;
        RHI::RHIPipelineState* m_BuildIndirectCmdPSO = nullptr;

        eastl::vector<RenderBatch> m_Instance;

        struct IndirectBatch
        {
            RHI::RHIPipelineState* PSO;
            uint32_t OriginMeshletListAddress;
            uint32_t OriginMeshletCount;
            uint32_t MeshletListBufferOffset;
        };
        eastl::vector<IndirectBatch> m_IndirectBatches;
        eastl::vector<RenderBatch> m_NonGPUDrivenBatches;

        uint32_t m_TotalInstanceCount = 0;
        uint32_t m_TotalMeshletCount = 0;
        uint32_t m_InstanceIndexAddress = 0;

        RG::RGHandle m_DiffuseRT;
        RG::RGHandle m_NormalRT;
        RG::RGHandle m_VelocityRT;
        RG::RGHandle m_DepthRT;
        
        RG::RGHandle m_2ndPhaseObjectListBuffer;
        RG::RGHandle m_2ndPhaseObjectListCounterBuffer;

        RG::RGHandle m_2ndPhaseMeshletListBuffer;
        RG::RGHandle m_2ndPhaseMeshletListCounterBuffer;
    };
}