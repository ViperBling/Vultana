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

        RG::RGHandle GetBasePassColorRT() const { return mBasePassColorRT; }
        RG::RGHandle GetBasePassDepthRT() const { return mBasePassDepthRT; }

    private:
        void MergeBatches();

        
        

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
        eastl::vector<RenderBatch> mNoGPUDrivenBatches;

        uint32_t mTotalInstanceCount = 0;
        uint32_t mTotalMeshletCount = 0;
        uint32_t mInstnaceIndexAddress = 0;

        RG::RGHandle mBasePassColorRT;
        RG::RGHandle mBasePassDepthRT;
    };
}