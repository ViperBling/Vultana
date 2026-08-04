#include "DeferredBasePass.hpp"
#include "Renderer/RendererBase.hpp"
#include "Renderer/RenderModules/HiZBuffer.hpp"

#include <EASTL/map.h>

namespace Renderer
{
    struct FClearCounterPassData
    {
        RG::FRGHandle FirstPhaseMeshletListCounterBuffer;
        RG::FRGHandle SecondPhaseObjectListCounterBuffer;
        RG::FRGHandle SecondPhaseMeshletListCounterBuffer;
    };

    struct FInstanceCullingData
    {
        RG::FRGHandle HZBTexture;
        RG::FRGHandle IndirectCommandBuffer;
        RG::FRGHandle CullingResultBuffer;
        RG::FRGHandle SecondPhaseObjectListBuffer;
        RG::FRGHandle SecondPhaseObjectListCounterBuffer;
    };

    struct FBuildMeshletListData
    {
        RG::FRGHandle CullingResultBuffer;
        RG::FRGHandle MeshletListBuffer;
        RG::FRGHandle MeshletListCounterBuffer;
    };

    struct FBuildIndirectCommandData
    {
        RG::FRGHandle MeshletListCounterBuffer;
        RG::FRGHandle IndirectCommandBuffer;
    };

    struct FBasePassData
    {
        RG::FRGHandle IndirectCommandBuffer;

        RG::FRGHandle InHZBTexture;
        RG::FRGHandle MeshletListBuffer;
        RG::FRGHandle MeshletListCounterBuffer;
        RG::FRGHandle OcclusionCulledMeshletsBuffer;
        RG::FRGHandle OcclusionCulledMeshletsCounterBuffer;

        RG::FRGHandle OutDiffuseRT;      // SRGB : diffuse(rgb) + ao(a)
        RG::FRGHandle OutNormalRT;       // RGBA8UNORM : world normal(xyz)
        RG::FRGHandle OutVelocityRT;     // RG16F : screen-space velocity
        RG::FRGHandle OutDepthRT;
    };

    static inline uint32_t RoundUpTo(uint32_t a, uint32_t b)
    {
        return (a / b + 1) * b;
    }

    FDeferredBasePass::FDeferredBasePass(FRendererBase *pRenderer) : m_pRenderer(pRenderer)
    {
        RHI::FRHIComputePipelineStateDesc computeDesc {};

        computeDesc.CS = pRenderer->GetShader("InstanceCulling.hlsl", "InstanceCulling", RHI::ERHIShaderType::CS, {"FIRST_PHASE=1"});
        m_InstanceCulling1stPhasePSO = pRenderer->GetPipelineState(computeDesc, "1st Phase Instance Culling PSO");

        computeDesc.CS = pRenderer->GetShader("InstanceCulling.hlsl", "InstanceCulling", RHI::ERHIShaderType::CS);
        m_InstanceCulling2ndPhasePSO = pRenderer->GetPipelineState(computeDesc, "2nd Phase Instance Culling PSO");

        computeDesc.CS = pRenderer->GetShader("InstanceCulling.hlsl", "BuildMeshletList", RHI::ERHIShaderType::CS);
        m_BuildMeshletListPSO = pRenderer->GetPipelineState(computeDesc, "Build Meshlet List PSO");

        computeDesc.CS = pRenderer->GetShader("InstanceCulling.hlsl", "BuildInstanceCullingCmd", RHI::ERHIShaderType::CS);
        m_BuildInstanceCullingCmdPSO = pRenderer->GetPipelineState(computeDesc, "Build Indirect Instance Culling Command PSO");

        computeDesc.CS = pRenderer->GetShader("InstanceCulling.hlsl", "BuildIndirectCmd", RHI::ERHIShaderType::CS);
        m_BuildIndirectCmdPSO = pRenderer->GetPipelineState(computeDesc, "Build Indirect Command PSO");
    }

    FRenderBatch &FDeferredBasePass::AddBatch()
    {
        FLinearAllocator* allocator = m_pRenderer->GetConstantAllocator();
        return m_Instance.emplace_back(*allocator);
    }

    void FDeferredBasePass::Render1stPhase(RG::FRenderGraph *pRenderGraph)
    {
        RENDER_GRAPH_EVENT(pRenderGraph, "BasePass: 1st Phase");

        MergeBatches();

        uint32_t maxDispatchNum = RoundUpTo((uint32_t)m_IndirectBatches.size(), 65536 / sizeof(uint32_t));
        uint32_t maxInstanceNum = RoundUpTo(m_pRenderer->GetInstanceCount(), 65536 / sizeof(uint8_t));
        uint32_t maxMeshletNum = RoundUpTo(m_TotalMeshletCount, 65536 / sizeof(uint2));

        FHiZBuffer *pHZB = m_pRenderer->GetHiZBuffer();

        auto clearCounterPass = pRenderGraph->AddPass<FClearCounterPassData>("Clear Counter", RG::RenderPassType::Compute,
            [&](FClearCounterPassData& data, RG::FRGBuilder& builder)
            {
                RHI::FRHIBufferDesc bufferDesc {};
                bufferDesc.Stride = 4;
                bufferDesc.Size = bufferDesc.Stride * maxDispatchNum;
                bufferDesc.Format = RHI::ERHIFormat::R32UI;
                bufferDesc.Usage = RHI::RHIBufferUsageTypedBuffer;

                data.FirstPhaseMeshletListCounterBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "FirstPhaseMeshletCounterBuffer");
                data.FirstPhaseMeshletListCounterBuffer = builder.Write(data.FirstPhaseMeshletListCounterBuffer);

                data.SecondPhaseObjectListCounterBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "SecondPhaseObjectListCounterBuffer");
                data.SecondPhaseObjectListCounterBuffer = builder.Write(data.SecondPhaseObjectListCounterBuffer);

                data.SecondPhaseMeshletListCounterBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "SecondPhaseMeshletListCounterBuffer");
                data.SecondPhaseMeshletListCounterBuffer = builder.Write(data.SecondPhaseMeshletListCounterBuffer);
            },
            [=](const FClearCounterPassData& data, RHI::FRHICommandList* pCmdList)
            {
                ResetCounter(pCmdList, 
                    pRenderGraph->GetBuffer(data.FirstPhaseMeshletListCounterBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListCounterBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseMeshletListCounterBuffer));
            });

        auto instanceCullingPass = pRenderGraph->AddPass<FInstanceCullingData>("Instance Culling", RG::RenderPassType::Compute, 
            [&](FInstanceCullingData &data, RG::FRGBuilder &builder)
            {
                RHI::FRHIBufferDesc bufferDesc;
                bufferDesc.Stride = 1;
                bufferDesc.Size = bufferDesc.Stride * maxInstanceNum;
                bufferDesc.Format = RHI::ERHIFormat::R8UI;
                bufferDesc.Usage = RHI::RHIBufferUsageTypedBuffer;
                data.CullingResultBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "FirstPhaseCullingResultBuffer");
                data.CullingResultBuffer = builder.Write(data.CullingResultBuffer);

                bufferDesc.Stride = 4;
                bufferDesc.Size = bufferDesc.Stride * maxInstanceNum;
                bufferDesc.Format = RHI::ERHIFormat::R32UI;
                data.SecondPhaseObjectListBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "SecondPhaseObjectListBuffer");
                data.SecondPhaseObjectListBuffer = builder.Write(data.SecondPhaseObjectListBuffer);

                data.SecondPhaseObjectListCounterBuffer = builder.Write(clearCounterPass->SecondPhaseObjectListCounterBuffer);

                for (uint32_t i = 0; i < pHZB->GetHZBMipCount(); i++)
                {
                    data.HZBTexture = builder.Read(pHZB->GetCullingHZBMip1stPhase(i), i, RG::RGBuilderFlag::None);
                }
            },
            [=](const FInstanceCullingData &data, RHI::FRHICommandList* pCmdList)
            {
                InstanceCulling1stPhase(pCmdList, 
                    pRenderGraph->GetBuffer(data.CullingResultBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListCounterBuffer));
            });
        
        auto buildMeshletListPass = pRenderGraph->AddPass<FBuildMeshletListData>("Build Meshlet List", RG::RenderPassType::Compute,
            [&](FBuildMeshletListData &data, RG::FRGBuilder &builder)
            {
                RHI::FRHIBufferDesc bufferDesc;
                bufferDesc.Stride = sizeof(uint2);
                bufferDesc.Size = bufferDesc.Stride * maxMeshletNum;
                bufferDesc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.MeshletListBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "FirstPhaseMeshletListBuffer");

                data.CullingResultBuffer = builder.Read(instanceCullingPass->CullingResultBuffer);
                data.MeshletListBuffer = builder.Write(data.MeshletListBuffer);
                data.MeshletListCounterBuffer = builder.Write(clearCounterPass->FirstPhaseMeshletListCounterBuffer);
            },
            [=](const FBuildMeshletListData &data, RHI::FRHICommandList* pCmdList)
            {
                BuildMeshletList(pCmdList, 
                    pRenderGraph->GetBuffer(data.CullingResultBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer));
            });
        
        auto buildIndirectCommandPass = pRenderGraph->AddPass<FBuildIndirectCommandData>("Build Indirect Command", RG::RenderPassType::Compute,
            [&](FBuildIndirectCommandData &data, RG::FRGBuilder &builder)
            {
                RHI::FRHIBufferDesc bufferDesc;
                bufferDesc.Stride = sizeof(uint3);
                bufferDesc.Size = bufferDesc.Stride * maxDispatchNum;
                bufferDesc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.IndirectCommandBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "FirstPhaseIndirectCommand");
                data.IndirectCommandBuffer = builder.Write(data.IndirectCommandBuffer);

                data.MeshletListCounterBuffer = builder.Read(buildMeshletListPass->MeshletListCounterBuffer);
            },
            [=](const FBuildIndirectCommandData &data, RHI::FRHICommandList* pCmdList)
            {
                BuildIndirectCommand(pCmdList, 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer), 
                    pRenderGraph->GetBuffer(data.IndirectCommandBuffer));
            });

        auto basePass = pRenderGraph->AddPass<FBasePassData>("Base Pass", RG::RenderPassType::Graphics,
            [&](FBasePassData& data, RG::FRGBuilder& builder)
            {
                RHI::FRHITextureDesc textureDesc {};
                textureDesc.Width = m_pRenderer->GetRenderWidth();
                textureDesc.Height = m_pRenderer->GetRenderHeight();

                textureDesc.Format = RHI::ERHIFormat::RGBA16F;
                data.OutDiffuseRT = builder.Create<RG::FRGTexture>(textureDesc, "BasePass_DiffuseRT");

                textureDesc.Format = RHI::ERHIFormat::RGBA8UNORM;
                data.OutNormalRT = builder.Create<RG::FRGTexture>(textureDesc, "BasePass_NormalRT");

                textureDesc.Format = RHI::ERHIFormat::RG16F;
                data.OutVelocityRT = builder.Create<RG::FRGTexture>(textureDesc, "BasePass_VelocityRT");

                textureDesc.Format = RHI::ERHIFormat::D32F;
                data.OutDepthRT = builder.Create<RG::FRGTexture>(textureDesc, "BasePass_DepthRT");

                data.OutDiffuseRT = builder.WriteColor(0, data.OutDiffuseRT, 0, RHI::ERHIRenderPassLoadOp::Clear, float4(0.0f));
                data.OutNormalRT = builder.WriteColor(1, data.OutNormalRT, 0, RHI::ERHIRenderPassLoadOp::Clear, float4(0.0f));
                data.OutVelocityRT = builder.WriteColor(2, data.OutVelocityRT, 0, RHI::ERHIRenderPassLoadOp::Clear, float4(0.0f));
                data.OutDepthRT = builder.WriteDepth(data.OutDepthRT, 0, RHI::ERHIRenderPassLoadOp::Clear, RHI::ERHIRenderPassLoadOp::Clear);

                for (uint32_t i = 0; i < pHZB->GetHZBMipCount(); ++i)
                {
                    data.InHZBTexture = builder.Read(pHZB->GetCullingHZBMip1stPhase(i), i, RG::RGBuilderFlag::ShaderStageNonPS);
                }

                data.IndirectCommandBuffer = builder.ReadIndirectArg(buildIndirectCommandPass->IndirectCommandBuffer);
                data.MeshletListBuffer = builder.Read(buildMeshletListPass->MeshletListBuffer, 0, RG::RGBuilderFlag::ShaderStageNonPS);
                data.MeshletListCounterBuffer = builder.Read(buildMeshletListPass->MeshletListCounterBuffer, 0, RG::RGBuilderFlag::ShaderStageNonPS);

                RHI::FRHIBufferDesc bufferDesc;
                bufferDesc.Stride = sizeof(uint2);
                bufferDesc.Size = bufferDesc.Stride * maxMeshletNum;
                bufferDesc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.OcclusionCulledMeshletsBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "SecondPhaseMeshletListBuffer");
                data.OcclusionCulledMeshletsBuffer = builder.Write(data.OcclusionCulledMeshletsBuffer);

                data.OcclusionCulledMeshletsCounterBuffer = builder.Write(clearCounterPass->SecondPhaseMeshletListCounterBuffer);
            },
            [=](const FBasePassData& data, RHI::FRHICommandList* pCmdList)
            {
                FlushBatches1stPhase(pCmdList, 
                    pRenderGraph->GetBuffer(data.IndirectCommandBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer));
            });

        m_DiffuseRT = basePass->OutDiffuseRT;
        m_NormalRT = basePass->OutNormalRT;
        m_VelocityRT = basePass->OutVelocityRT;
        m_DepthRT = basePass->OutDepthRT;

        m_2ndPhaseObjectListBuffer = instanceCullingPass->SecondPhaseObjectListBuffer;
        m_2ndPhaseObjectListCounterBuffer = instanceCullingPass->SecondPhaseObjectListCounterBuffer;

        m_2ndPhaseMeshletListBuffer = basePass->OcclusionCulledMeshletsBuffer;
        m_2ndPhaseMeshletListCounterBuffer = basePass->OcclusionCulledMeshletsCounterBuffer;
    }

    void FDeferredBasePass::Render2ndPhase(RG::FRenderGraph *pRenderGraph)
    {
        RENDER_GRAPH_EVENT(pRenderGraph, "BasePass: 2nd Phase");

        FHiZBuffer *pHZB = m_pRenderer->GetHiZBuffer();

        uint32_t maxDispatchNum = RoundUpTo((uint32_t)m_IndirectBatches.size(), 65536 / sizeof(uint32_t));
        uint32_t maxInstanceNum = RoundUpTo(m_pRenderer->GetInstanceCount(), 65536 / sizeof(uint8_t));

        struct FBuildCullingCommandData
        {
            RG::FRGHandle ObjectListCounterBuffer;
            RG::FRGHandle CommandBuffer;
        };

        auto buildCullingCommandPass = pRenderGraph->AddPass<FBuildCullingCommandData>("Build Instance Culling Command", RG::RenderPassType::Compute,
            [&](FBuildCullingCommandData& data, RG::FRGBuilder& builder)
            {
                RHI::FRHIBufferDesc desc;
                desc.Stride = sizeof(uint3);
                desc.Size = desc.Stride;
                desc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.CommandBuffer = builder.Create<RG::FRGBuffer>(desc, "SecondPhaseInstanceCullingCommandBuffer");
                data.CommandBuffer = builder.Write(data.CommandBuffer);

                data.ObjectListCounterBuffer = builder.Read(m_2ndPhaseObjectListCounterBuffer);
            },
            [=](const FBuildCullingCommandData &data, RHI::FRHICommandList* pCmdList)
            {
                RG::FRGBuffer* commandBuffer = pRenderGraph->GetBuffer(data.CommandBuffer);
                RG::FRGBuffer* objectListCounterBuffer = pRenderGraph->GetBuffer(data.ObjectListCounterBuffer);

                pCmdList->SetPipelineState(m_BuildInstanceCullingCmdPSO);

                uint32_t consts[2] = {commandBuffer->GetUAV()->GetHeapIndex(), objectListCounterBuffer->GetSRV()->GetHeapIndex()};
                pCmdList->SetComputeConstants(0, consts, sizeof(consts));
                pCmdList->Dispatch(1, 1, 1);
            });
        
        auto instanceCullingPass = pRenderGraph->AddPass<FInstanceCullingData>("Instance Culling", RG::RenderPassType::Compute,
            [&](FInstanceCullingData& data, RG::FRGBuilder& builder)
            {
                RHI::FRHIBufferDesc bufferDesc;
                bufferDesc.Stride = 1;
                bufferDesc.Size = bufferDesc.Stride * maxInstanceNum;
                bufferDesc.Format = RHI::ERHIFormat::R8UI;
                bufferDesc.Usage = RHI::RHIBufferUsageTypedBuffer;
                data.CullingResultBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "SecondPhaseCullingResultBuffer");
                data.CullingResultBuffer = builder.Write(data.CullingResultBuffer);

                data.IndirectCommandBuffer = builder.ReadIndirectArg(buildCullingCommandPass->CommandBuffer);
                data.SecondPhaseObjectListBuffer = builder.Read(m_2ndPhaseObjectListBuffer);
                data.SecondPhaseObjectListCounterBuffer = builder.Read(m_2ndPhaseObjectListCounterBuffer);

                for (uint32_t i = 0; i < pHZB->GetHZBMipCount(); ++i)
                {
                    data.HZBTexture = builder.Read(pHZB->GetCullingHZBMip2ndPhase(i), i, RG::RGBuilderFlag::None);
                }
            },
            [=](const FInstanceCullingData& data, RHI::FRHICommandList* pCmdList)
            {
                InstanceCulling2ndPhase(pCmdList, 
                    pRenderGraph->GetBuffer(data.IndirectCommandBuffer), 
                    pRenderGraph->GetBuffer(data.CullingResultBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListCounterBuffer));
            });
        
        auto buildMeshletListPass = pRenderGraph->AddPass<FBuildMeshletListData>("Build Meshlet List", RG::RenderPassType::Compute,
            [&](FBuildMeshletListData& data, RG::FRGBuilder& builder)
            {
                data.CullingResultBuffer = builder.Read(instanceCullingPass->CullingResultBuffer);
                data.MeshletListBuffer = builder.Write(m_2ndPhaseMeshletListBuffer);
                data.MeshletListCounterBuffer = builder.Write(m_2ndPhaseMeshletListCounterBuffer);
            },
            [=](const FBuildMeshletListData& data, RHI::FRHICommandList* pCmdList)
            {
                BuildMeshletList(pCmdList, 
                    pRenderGraph->GetBuffer(data.CullingResultBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer));
            });

        auto buildIndirectCommandPass = pRenderGraph->AddPass<FBuildIndirectCommandData>("Build Indirect Command", RG::RenderPassType::Compute,
            [&](FBuildIndirectCommandData& data, RG::FRGBuilder& builder)
            {
                RHI::FRHIBufferDesc bufferDesc;
                bufferDesc.Stride = sizeof(uint3);
                bufferDesc.Size = bufferDesc.Stride * maxDispatchNum;
                bufferDesc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.IndirectCommandBuffer = builder.Create<RG::FRGBuffer>(bufferDesc, "SecondPhaseIndirectCommand");
                data.IndirectCommandBuffer = builder.Write(data.IndirectCommandBuffer);

                data.MeshletListCounterBuffer = builder.Read(buildMeshletListPass->MeshletListCounterBuffer);
            },
            [=](const FBuildIndirectCommandData& data, RHI::FRHICommandList* pCmdList)
            {
                BuildIndirectCommand(pCmdList, 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer), 
                    pRenderGraph->GetBuffer(data.IndirectCommandBuffer));
            });
        
        auto basePass = pRenderGraph->AddPass<FBasePassData>("Base Pass", RG::RenderPassType::Graphics,
            [&](FBasePassData& data, RG::FRGBuilder& builder)
            {
                data.OutDiffuseRT = builder.WriteColor(0, m_DiffuseRT, 0, RHI::ERHIRenderPassLoadOp::Load);
                data.OutNormalRT = builder.WriteColor(1, m_NormalRT, 0, RHI::ERHIRenderPassLoadOp::Load);
                data.OutVelocityRT = builder.WriteColor(2, m_VelocityRT, 0, RHI::ERHIRenderPassLoadOp::Load);
                data.OutDepthRT = builder.WriteDepth(m_DepthRT, 0, RHI::ERHIRenderPassLoadOp::Load, RHI::ERHIRenderPassLoadOp::Load);

                for (uint32_t i = 0; i < pHZB->GetHZBMipCount(); ++i)
                {
                    data.InHZBTexture = builder.Read(pHZB->GetCullingHZBMip2ndPhase(i), i, RG::RGBuilderFlag::ShaderStageNonPS);
                }

                data.MeshletListBuffer = builder.Read(buildMeshletListPass->MeshletListBuffer, 0, RG::RGBuilderFlag::ShaderStageNonPS);
                data.MeshletListCounterBuffer = builder.Read(buildMeshletListPass->MeshletListCounterBuffer, 0, RG::RGBuilderFlag::ShaderStageNonPS);
                data.IndirectCommandBuffer = builder.ReadIndirectArg(buildIndirectCommandPass->IndirectCommandBuffer);
            },
            [=](const FBasePassData& data, RHI::FRHICommandList* pCmdList)
            {
                FlushBatches2ndPhase(pCmdList, 
                    pRenderGraph->GetBuffer(data.IndirectCommandBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer));
            });

        m_DiffuseRT = basePass->OutDiffuseRT;
        m_NormalRT = basePass->OutNormalRT;
        m_VelocityRT = basePass->OutVelocityRT;
        m_DepthRT = basePass->OutDepthRT;
    }

    void FDeferredBasePass::MergeBatches()
    {
        m_TotalInstanceCount = (uint32_t)m_Instance.size();

        eastl::vector<uint32_t> instanceIndices(m_TotalInstanceCount);
        for (uint32_t i = 0; i < m_TotalInstanceCount; ++i)
        {
            instanceIndices[i] = m_Instance[i].InstanceIndex;
        }
        m_InstanceIndexAddress = m_pRenderer->AllocateSceneConstantBuffer(instanceIndices.data(), sizeof(uint32_t) * m_TotalInstanceCount);

        m_TotalMeshletCount = 0;
        m_IndirectBatches.clear();
        m_NonGPUDrivenBatches.clear();

        struct FMergedBatch
        {
            eastl::vector<FRenderBatch*> Batches;
            uint32_t MeshletCount;
        };
        eastl::map<RHI::FRHIPipelineState*, FMergedBatch> mergedBatches;

        for (size_t i = 0; i < m_Instance.size(); ++i)
        {
            const FRenderBatch& batch = m_Instance[i];
            if (batch.PSO->GetType() == RHI::ERHIPipelineType::MeshShading)
            {
                m_TotalMeshletCount += batch.MeshletCount;

                auto iter = mergedBatches.find(batch.PSO);
                if (iter != mergedBatches.end())
                {
                    iter->second.MeshletCount += batch.MeshletCount;
                    iter->second.Batches.push_back(&m_Instance[i]);
                }
                else
                {
                    FMergedBatch mergeBatch;
                    mergeBatch.Batches.push_back(&m_Instance[i]);
                    mergeBatch.MeshletCount = batch.MeshletCount;
                    mergedBatches.insert(eastl::make_pair(batch.PSO, mergeBatch));
                }
            }
            else
            {
                m_NonGPUDrivenBatches.push_back(batch);     // TODO: also instance-culling & merging for VS batches
            }
        }

        uint32_t meshletListOffset = 0;
        for (auto iter = mergedBatches.begin(); iter != mergedBatches.end(); ++iter)
        {
            const FMergedBatch& batch = iter->second;

            eastl::vector<uint2> meshletList;
            meshletList.reserve(batch.MeshletCount);

            for (size_t i = 0; i < batch.Batches.size(); ++i)
            {
                uint32_t instanceIndex = batch.Batches[i]->InstanceIndex;
                for (uint32_t j = 0; j < batch.Batches[i]->MeshletCount; ++j)
                {
                    meshletList.emplace_back(instanceIndex, j);
                }
            }
            uint32_t meshletListAddress = m_pRenderer->AllocateSceneConstantBuffer(meshletList.data(), sizeof(uint2) * (uint32_t)meshletList.size());
            m_IndirectBatches.push_back({iter->first, meshletListAddress, batch.MeshletCount, meshletListOffset});
            meshletListOffset += batch.MeshletCount;
        }
        m_Instance.clear();
    }

    void FDeferredBasePass::ResetCounter(RHI::FRHICommandList* pCmdList, RG::FRGBuffer* firstPhaseMeshletCounter, RG::FRGBuffer* secondPhaseObjectCounter, RG::FRGBuffer* secondPhaseMeshletCounter)
    {
        uint32_t clearValue[4] = {0, 0, 0, 0};
        pCmdList->ClearUAV(firstPhaseMeshletCounter->GetBuffer(), firstPhaseMeshletCounter->GetUAV(), clearValue);
        pCmdList->ClearUAV(secondPhaseObjectCounter->GetBuffer(), secondPhaseObjectCounter->GetUAV(), clearValue);
        pCmdList->ClearUAV(secondPhaseMeshletCounter->GetBuffer(), secondPhaseMeshletCounter->GetUAV(), clearValue);

        pCmdList->BufferBarrier(firstPhaseMeshletCounter->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);
        pCmdList->BufferBarrier(secondPhaseObjectCounter->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);
        pCmdList->BufferBarrier(secondPhaseMeshletCounter->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);
    }

    void FDeferredBasePass::InstanceCulling1stPhase(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *cullingResultUAV, RG::FRGBuffer *secondPhaseObjectListUAV, RG::FRGBuffer *secondPhaseObjectListCounterUAV)
    {
        uint32_t clearValue[4] = {0, 0, 0, 0};
        pCmdList->ClearUAV(cullingResultUAV->GetBuffer(), cullingResultUAV->GetUAV(), clearValue);
        pCmdList->BufferBarrier(cullingResultUAV->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);

        pCmdList->SetPipelineState(m_InstanceCulling1stPhasePSO);

        uint32_t instanceCount = m_TotalInstanceCount;
        uint32_t rootConsts[5] = {
            m_InstanceIndexAddress,
            instanceCount,
            cullingResultUAV->GetUAV()->GetHeapIndex(),
            secondPhaseObjectListUAV->GetUAV()->GetHeapIndex(),
            secondPhaseObjectListCounterUAV->GetUAV()->GetHeapIndex()
        };
        pCmdList->SetComputeConstants(0, rootConsts, sizeof(rootConsts));

        uint32_t groupCount = eastl::max((instanceCount + 63) / 64, 1u);    // Avoid empty dispatch warning
        pCmdList->Dispatch(groupCount, 1, 1);
    }

    void FDeferredBasePass::InstanceCulling2ndPhase(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *pIndirectCommandBuffer, RG::FRGBuffer *cullingResultUAV, RG::FRGBuffer *objectListBufferSRV, RG::FRGBuffer *objectListCounterBufferSRV)
    {
        uint32_t clearValue[4] = {0, 0, 0, 0};
        pCmdList->ClearUAV(cullingResultUAV->GetBuffer(), cullingResultUAV->GetUAV(), clearValue);
        pCmdList->BufferBarrier(cullingResultUAV->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);

        pCmdList->SetPipelineState(m_InstanceCulling2ndPhasePSO);

        uint32_t rootConsts[3] = {objectListBufferSRV->GetSRV()->GetHeapIndex(), objectListCounterBufferSRV->GetSRV()->GetHeapIndex(), cullingResultUAV->GetUAV()->GetHeapIndex()};
        pCmdList->SetComputeConstants(0, rootConsts, sizeof(rootConsts));

        pCmdList->DispatchIndirect(pIndirectCommandBuffer->GetBuffer(), 0);
    }

    void FDeferredBasePass::FlushBatches1stPhase(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *pIndirectCommandBuffer, RG::FRGBuffer *pMeshletListSRV, RG::FRGBuffer *pMeshletListCounterSRV)
    {
        for (size_t i = 0; i < m_IndirectBatches.size(); ++i)
        {
            const FIndirectBatch &batch = m_IndirectBatches[i];
            pCmdList->SetPipelineState(batch.PSO);

            uint32_t rootConsts[5] = {pMeshletListSRV->GetSRV()->GetHeapIndex(), pMeshletListCounterSRV->GetSRV()->GetHeapIndex(), batch.MeshletListBufferOffset, (uint32_t)i, 1};
            pCmdList->SetGraphicsConstants(0, rootConsts, sizeof(rootConsts));

            pCmdList->DispatchMeshIndirect(pIndirectCommandBuffer->GetBuffer(), sizeof(uint3) * (uint32_t)i);
        }
    }

    void FDeferredBasePass::FlushBatches2ndPhase(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *pIndirectCommandBuffer, RG::FRGBuffer *pMeshletListSRV, RG::FRGBuffer *pMeshletListCounterSRV)
    {
        for (size_t i = 0; i < m_IndirectBatches.size(); ++i)
        {
            const FIndirectBatch &batch = m_IndirectBatches[i];
            pCmdList->SetPipelineState(batch.PSO);

            uint32_t rootConsts[5] = {pMeshletListSRV->GetSRV()->GetHeapIndex(), pMeshletListCounterSRV->GetSRV()->GetHeapIndex(), batch.MeshletListBufferOffset, (uint32_t)i, 0};
            pCmdList->SetGraphicsConstants(0, rootConsts, sizeof(rootConsts));

            pCmdList->DispatchMeshIndirect(pIndirectCommandBuffer->GetBuffer(), sizeof(uint3) * (uint32_t)i);
        }

        for (size_t i = 0; i < m_NonGPUDrivenBatches.size(); ++i)
        {
            DrawBatch(pCmdList, m_NonGPUDrivenBatches[i]);
        }
    }

    void FDeferredBasePass::BuildMeshletList(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *cullingResultSRV, RG::FRGBuffer *meshletListBufferUAV, RG::FRGBuffer *meshletListCounterBufferUAV)
    {
        pCmdList->SetPipelineState(m_BuildMeshletListPSO);

        for (size_t i = 0; i < m_IndirectBatches.size(); ++i)
        {
            uint32_t consts[7] = {
                (uint32_t)i,
                cullingResultSRV->GetSRV()->GetHeapIndex(),
                m_IndirectBatches[i].OriginMeshletListAddress,
                m_IndirectBatches[i].OriginMeshletCount,
                m_IndirectBatches[i].MeshletListBufferOffset,
                meshletListBufferUAV->GetUAV()->GetHeapIndex(),
                meshletListCounterBufferUAV->GetUAV()->GetHeapIndex()};
            pCmdList->SetComputeConstants(0, consts, sizeof(consts));
            pCmdList->Dispatch(DivideRoundingUp(m_IndirectBatches[i].OriginMeshletCount, 64), 1, 1);
        }
    }

    void FDeferredBasePass::BuildIndirectCommand(RHI::FRHICommandList *pCmdList, RG::FRGBuffer *pCounterBufferSRV, RG::FRGBuffer *pCommandBufferUAV)
    {
        pCmdList->SetPipelineState(m_BuildIndirectCmdPSO);

        uint32_t batchCount = (uint32_t)m_IndirectBatches.size();

        uint32_t consts[3] = {batchCount, pCounterBufferSRV->GetSRV()->GetHeapIndex(), pCommandBufferUAV->GetUAV()->GetHeapIndex()};
        pCmdList->SetComputeConstants(0, consts, sizeof(consts));

        uint32_t groupCount = eastl::max((batchCount + 63) / 64, 1u);   // Avoid empty dispatch warning
        pCmdList->Dispatch(groupCount, 1, 1);
    }
}