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

        RG::RGHandle GetDiffuseRT() const { return m_DiffuseRT; }
        RG::RGHandle GetSpecularRT() const { return m_SpecularRT; }
        RG::RGHandle GetNormalRT() const { return m_NormalRT; }
        RG::RGHandle GetEmissiveRT() const { return m_EmissiveRT; }
        RG::RGHandle GetCustomDataRT() const { return m_CustomDataRT; }
        RG::RGHandle GetDepthRT() const { return m_DepthRT; }

        RG::RGHandle GetMeshletListBuffer2ndPhase() const { return m_MeshletListBuffer2ndPhase; }
        RG::RGHandle GetMeshletListCounterBuffer2ndPhase() const { return m_MeshletListCounterBuffer2ndPhase; }

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

        uint32_t m_InstnaceIndexAddress = 0;

        RG::RGHandle m_DiffuseRT;
        RG::RGHandle m_SpecularRT;
        RG::RGHandle m_NormalRT;
        RG::RGHandle m_EmissiveRT;
        RG::RGHandle m_CustomDataRT;
        RG::RGHandle m_DepthRT;

        RG::RGHandle m_ObjectListBuffer2ndPhase;
        RG::RGHandle m_ObjectListCounterBuffer2ndPhase;

        RG::RGHandle m_MeshletListBuffer2ndPhase;
        RG::RGHandle m_MeshletListCounterBuffer2ndPhase;
    };
}