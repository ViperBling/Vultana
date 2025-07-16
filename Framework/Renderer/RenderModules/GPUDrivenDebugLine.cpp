#include "GPUDrivenDebugLine.hpp"
#include "Renderer/RendererBase.hpp"

#define MAX_VERTEX_COUNT 1024 * 1024

namespace Renderer
{
    GPUDrivenDebugLine::GPUDrivenDebugLine(Renderer::RendererBase * pRenderer)
    {
        m_pRenderer = pRenderer;

        RHI::RHIGraphicsPipelineStateDesc psoDesc {};
        psoDesc.VS = pRenderer->GetShader("GPUDrivenDebugLine.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        psoDesc.PS = pRenderer->GetShader("GPUDrivenDebugLine.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        psoDesc.DepthStencilState.bDepthTest = true;
        psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::Greater;
        psoDesc.DepthStencilState.bDepthWrite = false;
        psoDesc.RTFormats[0] = pRenderer->GetSwapchain()->GetDesc()->ColorFormat;
        psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;
        psoDesc.PrimitiveType = RHI::ERHIPrimitiveType::LineList;
        m_pPSO = pRenderer->GetPipelineState(psoDesc, "GPUDrivenDebugLine::m_pPSO");

        RHI::RHIDrawCommand cmd = {};
        cmd.InstanceCount = 1;
        m_pDrawArgsBuffer.reset(pRenderer->CreateRawBuffer(&cmd, sizeof(RHI::RHIDrawCommand), "GPUDrivenDebugLine::m_pDrawArgsBuffer", RHI::ERHIMemoryType::GPUOnly, true));

        const uint32_t lineVertexStride = 16;
        m_pLineVertexBuffer.reset(pRenderer->CreateStructuredBuffer(nullptr, lineVertexStride, MAX_VERTEX_COUNT, "GPUDrivenDebugLine::m_pLineVertexBuffer", RHI::ERHIMemoryType::GPUOnly, true));
    }

    void GPUDrivenDebugLine::Clear(RHI::RHICommandList *pCmdList)
    {
        GPU_EVENT_DEBUG(pCmdList, "GPUDrivenDebugLine::Clear");

        // Transition Buffer layout
        pCmdList->BufferBarrier(m_pDrawArgsBuffer->GetBuffer(), RHI::RHIAccessIndirectArgs, RHI::RHIAccessCopyDst);
        
        // Reset Buffer Data
        pCmdList->WriteBuffer(m_pDrawArgsBuffer->GetBuffer(), 0, 0);
        
        pCmdList->BufferBarrier(m_pDrawArgsBuffer->GetBuffer(), RHI::RHIAccessCopyDst, RHI::RHIAccessMaskUAV);
        pCmdList->BufferBarrier(m_pLineVertexBuffer->GetBuffer(), RHI::RHIAccessVertexShaderSRV, RHI::RHIAccessMaskUAV);
    }

    void GPUDrivenDebugLine::PrepareForDraw(RHI::RHICommandList *pCmdList)
    {
        pCmdList->BufferBarrier(m_pDrawArgsBuffer->GetBuffer(), RHI::RHIAccessMaskUAV, RHI::RHIAccessIndirectArgs);
        pCmdList->BufferBarrier(m_pLineVertexBuffer->GetBuffer(), RHI::RHIAccessMaskUAV, RHI::RHIAccessVertexShaderSRV);
    }

    void GPUDrivenDebugLine::Draw(RHI::RHICommandList *pCmdList)
    {
        GPU_EVENT_DEBUG(pCmdList, "GPUDrivenDebugLine::Draw");

        pCmdList->SetPipelineState(m_pPSO);
        
        uint rootConstants[1] = { GetVertexBufferSRV()->GetHeapIndex() };
        pCmdList->SetGraphicsConstants(0, rootConstants, sizeof(rootConstants));

        pCmdList->DrawIndirect(m_pDrawArgsBuffer->GetBuffer(), 0);
    }
}