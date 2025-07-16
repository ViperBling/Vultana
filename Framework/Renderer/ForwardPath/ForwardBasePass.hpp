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
        void Render(RG::RenderGraph* pRenderGraph);

        RG::RGHandle GetBasePassColorRT() const { return m_BasePassColorRT; }
        RG::RGHandle GetBasePassDepthRT() const { return m_BasePassDepthRT; }

    private:
        void MergeBatches();

        
        

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
        eastl::vector<RenderBatch> m_NoGPUDrivenBatches;

        uint32_t m_TotalInstanceCount = 0;
        uint32_t m_TotalMeshletCount = 0;
        uint32_t m_InstnaceIndexAddress = 0;

        RG::RGHandle m_BasePassColorRT;
        RG::RGHandle m_BasePassDepthRT;
    };
}