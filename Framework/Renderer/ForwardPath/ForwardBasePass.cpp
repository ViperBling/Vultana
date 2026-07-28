#include "ForwardBasePass.hpp"
#include "Renderer/RendererBase.hpp"
#include "Renderer/RenderModules/HiZBuffer.hpp"

#include <EASTL/map.h>

namespace Renderer
{
    struct FClearCounterPassData
    {
        RG::RGHandle FirstPhaseMeshletListCounterBuffer;
        RG::RGHandle SecondPhaseObjectListCounterBuffer;
        RG::RGHandle SecondPhaseMeshletListCounterBuffer;
    };

    struct FInstanceCullingData
    {
        RG::RGHandle HZBTexture;
        RG::RGHandle IndirectCommandBuffer;
        RG::RGHandle CullingResultBuffer;
        RG::RGHandle SecondPhaseObjectListBuffer;
        RG::RGHandle SecondPhaseObjectListCounterBuffer;
    };

    struct FBuildMeshletListData
    {
        RG::RGHandle CullingResultBuffer;
        RG::RGHandle MeshletListBuffer;
        RG::RGHandle MeshletListCounterBuffer;
    };

    struct FBuildIndirectCommandData
    {
        RG::RGHandle MeshletListCounterBuffer;
        RG::RGHandle IndirectCommandBuffer;
    };

    struct FBasePassData
    {
        RG::RGHandle IndirectCommandBuffer;

        RG::RGHandle InHZBTexture;
        RG::RGHandle MeshletListBuffer;
        RG::RGHandle MeshletListCounterBuffer;
        RG::RGHandle OcclusionCulledMeshletsBuffer;
        RG::RGHandle OcclusionCulledMeshletsCounterBuffer;

        RG::RGHandle OutDiffuseRT;      // SRGB : diffuse(rgb) + ao(a)
        RG::RGHandle OutNormalRT;       // RGBA8UNORM : world normal(xyz)
        RG::RGHandle OutVelocityRT;     // RG16F : screen-space velocity
        RG::RGHandle OutDepthRT;
    };

    static inline uint32_t RoundUpTo(uint32_t a, uint32_t b)
    {
        return (a / b + 1) * b;
    }

    ForwardBasePass::ForwardBasePass(RendererBase *pRenderer) : m_pRenderer(pRenderer)
    {
        RHI::RHIComputePipelineStateDesc computeDesc {};

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

    RenderBatch &ForwardBasePass::AddBatch()
    {
        LinearAllocator* allocator = m_pRenderer->GetConstantAllocator();
        return m_Instance.emplace_back(*allocator);
    }

    void ForwardBasePass::Render1stPhase(RG::RenderGraph *pRenderGraph)
    {
        RENDER_GRAPH_EVENT(pRenderGraph, "BasePass: 1st Phase");

        MergeBatches();

        uint32_t maxDispatchNum = RoundUpTo((uint32_t)m_IndirectBatches.size(), 65536 / sizeof(uint32_t));
        uint32_t maxInstanceNum = RoundUpTo(m_pRenderer->GetInstanceCount(), 65536 / sizeof(uint8_t));
        uint32_t maxMeshletNum = RoundUpTo(m_TotalMeshletCount, 65536 / sizeof(uint2));

        HiZBuffer *pHZB = m_pRenderer->GetHiZBuffer();

        auto clearCounterPass = pRenderGraph->AddPass<FClearCounterPassData>("Clear Counter", RG::RenderPassType::Compute,
            [&](FClearCounterPassData& data, RG::RGBuilder& builder)
            {
                RHI::RHIBufferDesc bufferDesc {};
                bufferDesc.Stride = 4;
                bufferDesc.Size = bufferDesc.Stride * maxDispatchNum;
                bufferDesc.Format = RHI::ERHIFormat::R32UI;
                bufferDesc.Usage = RHI::RHIBufferUsageTypedBuffer;

                data.FirstPhaseMeshletListCounterBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "FirstPhaseMeshletCounterBuffer");
                data.FirstPhaseMeshletListCounterBuffer = builder.Write(data.FirstPhaseMeshletListCounterBuffer);

                data.SecondPhaseObjectListCounterBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "SecondPhaseObjectListCounterBuffer");
                data.SecondPhaseObjectListCounterBuffer = builder.Write(data.SecondPhaseObjectListCounterBuffer);

                data.SecondPhaseMeshletListCounterBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "SecondPhaseMeshletListCounterBuffer");
                data.SecondPhaseMeshletListCounterBuffer = builder.Write(data.SecondPhaseMeshletListCounterBuffer);
            },
            [=](const FClearCounterPassData& data, RHI::RHICommandList* pCmdList)
            {
                ResetCounter(pCmdList, 
                    pRenderGraph->GetBuffer(data.FirstPhaseMeshletListCounterBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListCounterBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseMeshletListCounterBuffer));
            });

        auto instanceCullingPass = pRenderGraph->AddPass<FInstanceCullingData>("Instance Culling", RG::RenderPassType::Compute, 
            [&](FInstanceCullingData &data, RG::RGBuilder &builder)
            {
                RHI::RHIBufferDesc bufferDesc;
                bufferDesc.Stride = 1;
                bufferDesc.Size = bufferDesc.Stride * maxInstanceNum;
                bufferDesc.Format = RHI::ERHIFormat::R8UI;
                bufferDesc.Usage = RHI::RHIBufferUsageTypedBuffer;
                data.CullingResultBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "FirstPhaseCullingResultBuffer");
                data.CullingResultBuffer = builder.Write(data.CullingResultBuffer);

                bufferDesc.Stride = 4;
                bufferDesc.Size = bufferDesc.Stride * maxInstanceNum;
                bufferDesc.Format = RHI::ERHIFormat::R32UI;
                data.SecondPhaseObjectListBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "SecondPhaseObjectListBuffer");
                data.SecondPhaseObjectListBuffer = builder.Write(data.SecondPhaseObjectListBuffer);

                data.SecondPhaseObjectListCounterBuffer = builder.Write(clearCounterPass->SecondPhaseObjectListCounterBuffer);

                for (uint32_t i = 0; i < pHZB->GetHZBMipCount(); i++)
                {
                    data.HZBTexture = builder.Read(pHZB->GetCullingHZBMip1stPhase(i), i, RG::RGBuilderFlag::None);
                }
            },
            [=](const FInstanceCullingData &data, RHI::RHICommandList* pCmdList)
            {
                InstanceCulling1stPhase(pCmdList, 
                    pRenderGraph->GetBuffer(data.CullingResultBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListCounterBuffer));
            });
        
        auto buildMeshletListPass = pRenderGraph->AddPass<FBuildMeshletListData>("Build Meshlet List", RG::RenderPassType::Compute,
            [&](FBuildMeshletListData &data, RG::RGBuilder &builder)
            {
                RHI::RHIBufferDesc bufferDesc;
                bufferDesc.Stride = sizeof(uint2);
                bufferDesc.Size = bufferDesc.Stride * maxMeshletNum;
                bufferDesc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.MeshletListBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "FirstPhaseMeshletListBuffer");

                data.CullingResultBuffer = builder.Read(instanceCullingPass->CullingResultBuffer);
                data.MeshletListBuffer = builder.Write(data.MeshletListBuffer);
                data.MeshletListCounterBuffer = builder.Write(clearCounterPass->FirstPhaseMeshletListCounterBuffer);
            },
            [=](const FBuildMeshletListData &data, RHI::RHICommandList* pCmdList)
            {
                BuildMeshletList(pCmdList, 
                    pRenderGraph->GetBuffer(data.CullingResultBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer));
            });
        
        auto buildIndirectCommandPass = pRenderGraph->AddPass<FBuildIndirectCommandData>("Build Indirect Command", RG::RenderPassType::Compute,
            [&](FBuildIndirectCommandData &data, RG::RGBuilder &builder)
            {
                RHI::RHIBufferDesc bufferDesc;
                bufferDesc.Stride = sizeof(uint3);
                bufferDesc.Size = bufferDesc.Stride * maxDispatchNum;
                bufferDesc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.IndirectCommandBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "FirstPhaseIndirectCommand");
                data.IndirectCommandBuffer = builder.Write(data.IndirectCommandBuffer);

                data.MeshletListCounterBuffer = builder.Read(buildMeshletListPass->MeshletListCounterBuffer);
            },
            [=](const FBuildIndirectCommandData &data, RHI::RHICommandList* pCmdList)
            {
                BuildIndirectCommand(pCmdList, 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer), 
                    pRenderGraph->GetBuffer(data.IndirectCommandBuffer));
            });

        auto basePass = pRenderGraph->AddPass<FBasePassData>("Base Pass", RG::RenderPassType::Graphics,
            [&](FBasePassData& data, RG::RGBuilder& builder)
            {
                RHI::RHITextureDesc textureDesc {};
                textureDesc.Width = m_pRenderer->GetRenderWidth();
                textureDesc.Height = m_pRenderer->GetRenderHeight();

                textureDesc.Format = RHI::ERHIFormat::RGBA8SRGB;
                data.OutDiffuseRT = builder.Create<RG::RGTexture>(textureDesc, "BasePass_DiffuseRT");

                textureDesc.Format = RHI::ERHIFormat::RGBA8UNORM;
                data.OutNormalRT = builder.Create<RG::RGTexture>(textureDesc, "BasePass_NormalRT");

                textureDesc.Format = RHI::ERHIFormat::RG16F;
                data.OutVelocityRT = builder.Create<RG::RGTexture>(textureDesc, "BasePass_VelocityRT");

                textureDesc.Format = RHI::ERHIFormat::D32F;
                data.OutDepthRT = builder.Create<RG::RGTexture>(textureDesc, "BasePass_DepthRT");

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

                RHI::RHIBufferDesc bufferDesc;
                bufferDesc.Stride = sizeof(uint2);
                bufferDesc.Size = bufferDesc.Stride * maxMeshletNum;
                bufferDesc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.OcclusionCulledMeshletsBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "SecondPhaseMeshletListBuffer");
                data.OcclusionCulledMeshletsBuffer = builder.Write(data.OcclusionCulledMeshletsBuffer);

                data.OcclusionCulledMeshletsCounterBuffer = builder.Write(clearCounterPass->SecondPhaseMeshletListCounterBuffer);
            },
            [=](const FBasePassData& data, RHI::RHICommandList* pCmdList)
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

    void ForwardBasePass::Render2ndPhase(RG::RenderGraph *pRenderGraph)
    {
        RENDER_GRAPH_EVENT(pRenderGraph, "BasePass: 2nd Phase");

        HiZBuffer *pHZB = m_pRenderer->GetHiZBuffer();

        uint32_t maxDispatchNum = RoundUpTo((uint32_t)m_IndirectBatches.size(), 65536 / sizeof(uint32_t));
        uint32_t maxInstanceNum = RoundUpTo(m_pRenderer->GetInstanceCount(), 65536 / sizeof(uint8_t));

        struct FBuildCullingCommandData
        {
            RG::RGHandle ObjectListCounterBuffer;
            RG::RGHandle CommandBuffer;
        };

        auto buildCullingCommandPass = pRenderGraph->AddPass<FBuildCullingCommandData>("Build Instance Culling Command", RG::RenderPassType::Compute,
            [&](FBuildCullingCommandData& data, RG::RGBuilder& builder)
            {
                RHI::RHIBufferDesc desc;
                desc.Stride = sizeof(uint3);
                desc.Size = desc.Stride;
                desc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.CommandBuffer = builder.Create<RG::RGBuffer>(desc, "SecondPhaseInstanceCullingCommandBuffer");
                data.CommandBuffer = builder.Write(data.CommandBuffer);

                data.ObjectListCounterBuffer = builder.Read(m_2ndPhaseObjectListCounterBuffer);
            },
            [=](const FBuildCullingCommandData &data, RHI::RHICommandList* pCmdList)
            {
                RG::RGBuffer* commandBuffer = pRenderGraph->GetBuffer(data.CommandBuffer);
                RG::RGBuffer* objectListCounterBuffer = pRenderGraph->GetBuffer(data.ObjectListCounterBuffer);

                pCmdList->SetPipelineState(m_BuildInstanceCullingCmdPSO);

                uint32_t consts[2] = {commandBuffer->GetUAV()->GetHeapIndex(), objectListCounterBuffer->GetUAV()->GetHeapIndex()};
                pCmdList->SetComputeConstants(0, consts, sizeof(consts));
                pCmdList->Dispatch(1, 1, 1);
            });
        
        auto instanceCullingPass = pRenderGraph->AddPass<FInstanceCullingData>("Instance Culling", RG::RenderPassType::Compute,
            [&](FInstanceCullingData& data, RG::RGBuilder& builder)
            {
                RHI::RHIBufferDesc bufferDesc;
                bufferDesc.Stride = 1;
                bufferDesc.Size = bufferDesc.Stride * maxInstanceNum;
                bufferDesc.Format = RHI::ERHIFormat::R8UI;
                bufferDesc.Usage = RHI::RHIBufferUsageTypedBuffer;
                data.CullingResultBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "SecondPhaseCullingResultBuffer");
                data.CullingResultBuffer = builder.Write(data.CullingResultBuffer);

                data.IndirectCommandBuffer = builder.ReadIndirectArg(buildCullingCommandPass->CommandBuffer);
                data.SecondPhaseObjectListBuffer = builder.Read(m_2ndPhaseObjectListBuffer);
                data.SecondPhaseObjectListCounterBuffer = builder.Read(m_2ndPhaseObjectListCounterBuffer);

                for (uint32_t i = 0; i < pHZB->GetHZBMipCount(); ++i)
                {
                    data.HZBTexture = builder.Read(pHZB->GetCullingHZBMip2ndPhase(i), i, RG::RGBuilderFlag::None);
                }
            },
            [=](const FInstanceCullingData& data, RHI::RHICommandList* pCmdList)
            {
                InstanceCulling2ndPhase(pCmdList, 
                    pRenderGraph->GetBuffer(data.IndirectCommandBuffer), 
                    pRenderGraph->GetBuffer(data.CullingResultBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListBuffer), 
                    pRenderGraph->GetBuffer(data.SecondPhaseObjectListCounterBuffer));
            });
        
        auto buildMeshletListPass = pRenderGraph->AddPass<FBuildMeshletListData>("Build Meshlet List", RG::RenderPassType::Compute,
            [&](FBuildMeshletListData& data, RG::RGBuilder& builder)
            {
                data.CullingResultBuffer = builder.Read(instanceCullingPass->CullingResultBuffer);
                data.MeshletListBuffer = builder.Write(m_2ndPhaseMeshletListBuffer);
                data.MeshletListCounterBuffer = builder.Write(m_2ndPhaseMeshletListCounterBuffer);
            },
            [=](const FBuildMeshletListData& data, RHI::RHICommandList* pCmdList)
            {
                BuildMeshletList(pCmdList, 
                    pRenderGraph->GetBuffer(data.CullingResultBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListBuffer), 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer));
            });

        auto buildIndirectCommandPass = pRenderGraph->AddPass<FBuildIndirectCommandData>("Build Indirect Command", RG::RenderPassType::Compute,
            [&](FBuildIndirectCommandData& data, RG::RGBuilder& builder)
            {
                RHI::RHIBufferDesc bufferDesc;
                bufferDesc.Stride = sizeof(uint3);
                bufferDesc.Size = bufferDesc.Stride * maxDispatchNum;
                bufferDesc.Usage = RHI::RHIBufferUsageStructuredBuffer;
                data.IndirectCommandBuffer = builder.Create<RG::RGBuffer>(bufferDesc, "SecondPhaseIndirectCommand");
                data.IndirectCommandBuffer = builder.Write(data.IndirectCommandBuffer);

                data.MeshletListCounterBuffer = builder.Read(buildMeshletListPass->MeshletListCounterBuffer);
            },
            [=](const FBuildIndirectCommandData& data, RHI::RHICommandList* pCmdList)
            {
                BuildIndirectCommand(pCmdList, 
                    pRenderGraph->GetBuffer(data.MeshletListCounterBuffer), 
                    pRenderGraph->GetBuffer(data.IndirectCommandBuffer));
            });
        
        auto basePass = pRenderGraph->AddPass<FBasePassData>("Base Pass", RG::RenderPassType::Graphics,
            [&](FBasePassData& data, RG::RGBuilder& builder)
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
            [=](const FBasePassData& data, RHI::RHICommandList* pCmdList)
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

    void ForwardBasePass::MergeBatches()
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
            eastl::vector<RenderBatch*> Batches;
            uint32_t MeshletCount;
        };
        eastl::map<RHI::RHIPipelineState*, FMergedBatch> mergedBatches;

        for (size_t i = 0; i < m_Instance.size(); ++i)
        {
            const RenderBatch& batch = m_Instance[i];
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

    void ForwardBasePass::ResetCounter(RHI::RHICommandList* pCmdList, RG::RGBuffer* firstPhaseMeshletCounter, RG::RGBuffer* secondPhaseObjectCounter, RG::RGBuffer* secondPhaseMeshletCounter)
    {
        uint32_t clearValue[4] = {0, 0, 0, 0};
        pCmdList->ClearUAV(firstPhaseMeshletCounter->GetBuffer(), firstPhaseMeshletCounter->GetUAV(), clearValue);
        pCmdList->ClearUAV(secondPhaseObjectCounter->GetBuffer(), secondPhaseObjectCounter->GetUAV(), clearValue);
        pCmdList->ClearUAV(secondPhaseMeshletCounter->GetBuffer(), secondPhaseMeshletCounter->GetUAV(), clearValue);

        pCmdList->BufferBarrier(firstPhaseMeshletCounter->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);
        pCmdList->BufferBarrier(secondPhaseObjectCounter->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);
        pCmdList->BufferBarrier(secondPhaseMeshletCounter->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);
    }

    void ForwardBasePass::InstanceCulling1stPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *cullingResultUAV, RG::RGBuffer *secondPhaseObjectListUAV, RG::RGBuffer *secondPhaseObjectListCounterUAV)
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

    void ForwardBasePass::InstanceCulling2ndPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCommandBuffer, RG::RGBuffer *cullingResultUAV, RG::RGBuffer *objectListBufferSRV, RG::RGBuffer *objectListCounterBufferSRV)
    {
        uint32_t clearValue[4] = {0, 0, 0, 0};
        pCmdList->ClearUAV(cullingResultUAV->GetBuffer(), cullingResultUAV->GetUAV(), clearValue);
        pCmdList->BufferBarrier(cullingResultUAV->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessComputeUAV);

        pCmdList->SetPipelineState(m_InstanceCulling2ndPhasePSO);

        uint32_t rootConsts[3] = {objectListBufferSRV->GetSRV()->GetHeapIndex(), objectListCounterBufferSRV->GetSRV()->GetHeapIndex(), cullingResultUAV->GetUAV()->GetHeapIndex()};
        pCmdList->SetComputeConstants(0, rootConsts, sizeof(rootConsts));

        pCmdList->DispatchIndirect(pIndirectCommandBuffer->GetBuffer(), 0);
    }

    void ForwardBasePass::FlushBatches1stPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCommandBuffer, RG::RGBuffer *pMeshletListSRV, RG::RGBuffer *pMeshletListCounterSRV)
    {
        for (size_t i = 0; i < m_IndirectBatches.size(); ++i)
        {
            const IndirectBatch &batch = m_IndirectBatches[i];
            pCmdList->SetPipelineState(batch.PSO);

            uint32_t rootConsts[5] = {pMeshletListSRV->GetSRV()->GetHeapIndex(), pMeshletListCounterSRV->GetSRV()->GetHeapIndex(), batch.MeshletListBufferOffset, (uint32_t)i, 1};
            pCmdList->SetGraphicsConstants(0, rootConsts, sizeof(rootConsts));

            pCmdList->DispatchMeshIndirect(pIndirectCommandBuffer->GetBuffer(), sizeof(uint3) * (uint32_t)i);
        }
    }

    void ForwardBasePass::FlushBatches2ndPhase(RHI::RHICommandList *pCmdList, RG::RGBuffer *pIndirectCommandBuffer, RG::RGBuffer *pMeshletListSRV, RG::RGBuffer *pMeshletListCounterSRV)
    {
        for (size_t i = 0; i < m_IndirectBatches.size(); ++i)
        {
            const IndirectBatch &batch = m_IndirectBatches[i];
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

    void ForwardBasePass::BuildMeshletList(RHI::RHICommandList *pCmdList, RG::RGBuffer *cullingResultSRV, RG::RGBuffer *meshletListBufferUAV, RG::RGBuffer *meshletListCounterBufferUAV)
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

    void ForwardBasePass::BuildIndirectCommand(RHI::RHICommandList *pCmdList, RG::RGBuffer *pCounterBufferSRV, RG::RGBuffer *pCommandBufferUAV)
    {
        pCmdList->SetPipelineState(m_BuildIndirectCmdPSO);

        uint32_t batchCount = (uint32_t)m_IndirectBatches.size();

        uint32_t consts[3] = {batchCount, pCounterBufferSRV->GetSRV()->GetHeapIndex(), pCommandBufferUAV->GetUAV()->GetHeapIndex()};
        pCmdList->SetComputeConstants(0, consts, sizeof(consts));

        uint32_t groupCount = eastl::max((batchCount + 63) / 64, 1u);   // Avoid empty dispatch warning
        pCmdList->Dispatch(groupCount, 1, 1);
    }
}