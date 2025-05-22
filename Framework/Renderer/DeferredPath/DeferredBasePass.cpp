#include "DeferredBasePass.hpp"
#include "Renderer/RendererBase.hpp"

namespace Renderer
{
    struct FInstanceCullingData1stPhase
    {
        RG::RGHandle ObjectListBuffer;
        RG::RGHandle VisibleObjectListBuffer;
        RG::RGHandle CulledObjectListBuffer;
    };

    struct FInstanceCullingData2ndPhase
    {
        RG::RGHandle ObjectListBuffer;
        RG::RGHandle VisibleObjectListBuffer;
    };

    struct FDeferredBasePassData
    {
        RG::RGHandle IndirectCmdBuffer;

        RG::RGHandle InHZB;
        RG::RGHandle MeshletListBuffer;
        RG::RGHandle MeshletListCounterBuffer;
        RG::RGHandle OcclusionCulledMeshletsBuffer;             // Used for 1st phase
        RG::RGHandle OcclusionCulledMeshletsCounterBuffer;      // Used for 1st phase

        RG::RGHandle OutDiffuseRT;                              // SRGB : Diffuse(RGB) + AO(A)
        RG::RGHandle OutSpecularRT;                             // SRGB : Specular(RGB) + ShadingModeID(A)
        RG::RGHandle OutNormalRT;                               // RGBA8UNORM : Normal(RGB) + Roughness(A)
        RG::RGHandle OutEmissiveRT;                             // R11G11B10F : Emissive
        RG::RGHandle OutCustomDataRT;                           // RGBA8NORM : CustomData
        RG::RGHandle OutDepthRT;
    };

    DeferredBasePass::DeferredBasePass(RendererBase *pRenderer)
    {
        mpRenderer = pRenderer;
    }

    RenderBatch &DeferredBasePass::AddBatch()
    {
        LinearAllocator* allocator = mpRenderer->GetConstantAllocator();
        return mInstance.emplace_back(*allocator);
    }

    void DeferredBasePass::Render1stPhase(RG::RenderGraph *pRG)
    {
    }

    void DeferredBasePass::Render2ndPhase(RG::RenderGraph *pRG)
    {
    }

    void DeferredBasePass::MergeBatches()
    {
    }

    void DeferredBasePass::ResetCounter(RHI::RHICommandList *pCmdList, RG::RGBuffer *meshletCounter1stPhase, RG::RGBuffer *objectCounter2ndPhase, RG::RGBuffer *meshletCounter2ndPhase)
    {
    }

    void DeferredBasePass::InstanceCulling1stPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *cullingResultUAV, RG::RGBuffer *ObjectListUAV2ndPhase, RG::RGBuffer *ObjectListCounterUAV2ndPhase)
    {
    }

    void DeferredBasePass::InstanceCulling2ndPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCmdBuffer, RG::RGBuffer *cullingResultUAV, RG::RGBuffer *objectListBufferSRV, RG::RGBuffer *objectListCounterBufferSRV)
    {
    }

    void DeferredBasePass::FlushBatches1stPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCmdBuffer, RG::RGBuffer *pMeshletListSRV, RG::RGBuffer *pMeshletListCounterSRV)
    {
    }

    void DeferredBasePass::FlushBatches2ndPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCmdBuffer, RG::RGBuffer *pMeshletListSRV, RG::RGBuffer *pMeshletListCounterSRV)
    {
    }

    void DeferredBasePass::BuildMeshletList(RHI::RHICommandList *pCmdList, RG::RGBuffer *cullingResultSRV, RG::RGBuffer *meshletListBufferUAV, RG::RGBuffer *meshletListCounterBufferUAV)
    {
    }

    void DeferredBasePass::BuildIndirectCommand(RHI::RHICommandList *pCmdList, RG::RGBuffer *pCounterBufferSRV, RG::RGBuffer *pIndirectCmdBufferUAV)
    {
    }
}