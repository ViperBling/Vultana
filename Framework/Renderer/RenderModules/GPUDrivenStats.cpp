#include "GPUDrivenStats.hpp"
#include "Renderer/RendererBase.hpp"
#include "Common/Stats.hlsli"

#include <imgui.h>

namespace Renderer
{
    GPUDrivenStats::GPUDrivenStats(RendererBase *pRenderer) : m_pRenderer(pRenderer)
    {
        // --- Stats accumulation buffer (GPU-only, UAV) ---
        m_pStatsBuffer = eastl::make_unique<RenderResources::TypedBuffer>("GPUDrivenStats::m_pStatsBuffer");
        m_pStatsBuffer->Create(RHI::ERHIFormat::R32UI, STATS_MAX_TYPE_COUNT, RHI::ERHIMemoryType::GPUOnly, true);

        // --- Per-inflight-frame readback buffers (GPU-to-CPU) ---
        RHI::RHIDevice *pDevice = pRenderer->GetDevice();
        for (uint32_t i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; ++i)
        {
            RHI::RHIBufferDesc desc;
            desc.Stride = sizeof(uint32_t);
            desc.Size = sizeof(uint32_t) * STATS_MAX_TYPE_COUNT;
            desc.Format = RHI::ERHIFormat::R32UI;
            desc.MemoryType = RHI::ERHIMemoryType::GPUToCPU;
            desc.Usage = RHI::RHIBufferUsageTypedBuffer;

            eastl::string name = "GPUDrivenStats::m_pReadbackBuffer[";
            name.append(eastl::to_string(i));
            name.append("]");

            m_pReadbackBuffers[i].reset(pDevice->CreateBuffer(desc, name));
        }

        memset(m_ReadbackValues, 0, sizeof(m_ReadbackValues));
    }

    void GPUDrivenStats::Clear(RHI::RHICommandList *pCmdList)
    {
        GPU_EVENT_DEBUG(pCmdList, "GPUDrivenStats::Clear");

        pCmdList->BufferBarrier(m_pStatsBuffer->GetBuffer(), RHI::RHIAccessComputeSRV, RHI::RHIAccessClearUAV);

        uint32_t clearValue[4] = {0, 0, 0, 0};
        pCmdList->ClearUAV(m_pStatsBuffer->GetBuffer(), m_pStatsBuffer->GetUAV(), clearValue);

        pCmdList->BufferBarrier(m_pStatsBuffer->GetBuffer(), RHI::RHIAccessClearUAV, RHI::RHIAccessMaskUAV);
    }

    void GPUDrivenStats::Readback(RHI::RHICommandList *pCmdList)
    {
        GPU_EVENT_DEBUG(pCmdList, "GPUDrivenStats::Readback");

        uint32_t frameIndex = m_pRenderer->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHIBuffer *pReadback = m_pReadbackBuffers[frameIndex].get();

        // Transition stats buffer UAV → CopySrc
        pCmdList->BufferBarrier(m_pStatsBuffer->GetBuffer(), RHI::RHIAccessMaskUAV, RHI::RHIAccessCopySrc);

        // Transition readback buffer to CopyDst
        pCmdList->BufferBarrier(pReadback, RHI::RHIAccessCopySrc, RHI::RHIAccessCopyDst);

        pCmdList->CopyBuffer(m_pStatsBuffer->GetBuffer(), pReadback, 0, 0, sizeof(uint32_t) * STATS_MAX_TYPE_COUNT);

        // Transition readback buffer back so CPU can read it
        pCmdList->BufferBarrier(pReadback, RHI::RHIAccessCopyDst, RHI::RHIAccessCopySrc);

        // Restore stats buffer for next frame's UAV writes
        pCmdList->BufferBarrier(m_pStatsBuffer->GetBuffer(), RHI::RHIAccessCopySrc, RHI::RHIAccessMaskUAV);

        // Read CPU data from a completed readback buffer (2 frames behind is safe)
        uint32_t readFrameIndex = (frameIndex + RHI::RHI_MAX_INFLIGHT_FRAMES - 2) % RHI::RHI_MAX_INFLIGHT_FRAMES;
        RHI::RHIBuffer *pReadable = m_pReadbackBuffers[readFrameIndex].get();
        const uint32_t *pData = static_cast<const uint32_t *>(pReadable->GetCPUAddress());
        if (pData)
        {
            memcpy(m_ReadbackValues, pData, sizeof(uint32_t) * 16);
        }
    }

    void GPUDrivenStats::OnGui()
    {
        if (!ImGui::Begin("GPU Driven Stats", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        struct StatEntry
        {
            const char *Label;
            uint32_t Index;
        };

        ImGui::Text("1st Phase");
        ImGui::Separator();

        {
            StatEntry entries[] = {
                {"Objects Culled", STATS_1ST_PHASE_CULLED_OBJECTS},
                {"Objects Rendered", STATS_1ST_PHASE_RENDERED_OBJECTS},
            };
            for (auto &e : entries)
                ImGui::Text("  %-22s %u", e.Label, GetCounterValue(e.Index));
        }

        ImGui::Spacing();
        ImGui::Text("1st Phase Meshlets");
        ImGui::Separator();

        {
            StatEntry entries[] = {
                {"Frustum Culled", STATS_1ST_PHASE_FRUSTUM_CULLED_MESHLET},
                {"Backface Culled", STATS_1ST_PHASE_BACKFACE_CULLED_MESHLET},
                {"Occlusion Culled", STATS_1ST_PHASE_OCCLUSION_CULLED_MESHLET},
                {"Rendered", STATS_1ST_PHASE_RENDERED_MESHLET},
            };
            for (auto &e : entries)
                ImGui::Text("  %-22s %u", e.Label, GetCounterValue(e.Index));
        }

        ImGui::Spacing();
        ImGui::Text("1st Phase Triangles");
        ImGui::Separator();

        {
            StatEntry entries[] = {
                {"Culled", STATS_1ST_PHASE_CULLED_TRIANGLE},
                {"Rendered", STATS_1ST_PHASE_RENDERED_TRIANGLE},
            };
            for (auto &e : entries)
                ImGui::Text("  %-22s %u", e.Label, GetCounterValue(e.Index));
        }

        ImGui::Spacing();
        ImGui::Text("2nd Phase");
        ImGui::Separator();

        {
            StatEntry entries[] = {
                {"Objects Culled", STATS_2ND_PHASE_CULLED_OBJECTS},
                {"Objects Rendered", STATS_2ND_PHASE_RENDERED_OBJECTS},
            };
            for (auto &e : entries)
                ImGui::Text("  %-22s %u", e.Label, GetCounterValue(e.Index));
        }

        ImGui::Spacing();
        ImGui::Text("2nd Phase Meshlets");
        ImGui::Separator();

        {
            StatEntry entries[] = {
                {"Frustum Culled", STATS_2ND_PHASE_FRUSTUM_CULLED_MESHLET},
                {"Backface Culled", STATS_2ND_PHASE_BACKFACE_CULLED_MESHLET},
                {"Occlusion Culled", STATS_2ND_PHASE_OCCLUSION_CULLED_MESHLET},
                {"Rendered", STATS_2ND_PHASE_RENDERED_MESHLET},
            };
            for (auto &e : entries)
                ImGui::Text("  %-22s %u", e.Label, GetCounterValue(e.Index));
        }

        ImGui::Spacing();
        ImGui::Text("2nd Phase Triangles");
        ImGui::Separator();

        {
            StatEntry entries[] = {
                {"Culled", STATS_2ND_PHASE_CULLED_TRIANGLE},
                {"Rendered", STATS_2ND_PHASE_RENDERED_TRIANGLE},
            };
            for (auto &e : entries)
                ImGui::Text("  %-22s %u", e.Label, GetCounterValue(e.Index));
        }

        ImGui::End();
    }

    uint32_t GPUDrivenStats::GetCounterValue(uint32_t index) const
    {
        return (index < 16) ? m_ReadbackValues[index] : 0;
    }
}