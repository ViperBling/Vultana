#include "ImGuiImplement.hpp"

#include "Core/VultanaEngine.hpp"

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGuizmo/ImGuizmo.h>

namespace Editor
{
    ImGuiImplement::ImGuiImplement(Renderer::RendererBase* pRenderer) : mpRenderer(pRenderer)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking

        io.BackendRendererName = "imgui_impl_vultana";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(Core::VultanaEngine::GetEngineInstance()->GetWindowHandle());
    }

    ImGuiImplement::~ImGuiImplement()
    {
        ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();
    }

    bool ImGuiImplement::Init()
    {
        auto pDevice = mpRenderer->GetDevice();

        eastl::string iniPath = Core::VultanaEngine::GetEngineInstance()->GetWorkingPath() + "Config/ImGui.ini";
        ImGui::LoadIniSettingsFromDisk(iniPath.c_str());

        float scaling = ImGui_ImplWin32_GetDpiScaleForHwnd(Core::VultanaEngine::GetEngineInstance()->GetWindowHandle());
        ImGui::GetStyle().ScaleAllSizes(scaling);

        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = scaling;

        ImFontConfig fontConfig;
        fontConfig.OversampleH = fontConfig.OversampleV = 3;

        eastl::string fontFile = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath() + "Fonts/DroidSans.ttf";
        io.Fonts->AddFontFromFileTTF(fontFile.c_str(), 13.0f, &fontConfig);

        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        mpFontTexture.reset(mpRenderer->CreateTexture2D(width, height, 1, RHI::ERHIFormat::RGBA8UNORM, 0, "GUI::FontTexture"));
        mpRenderer->UploadTexture(mpFontTexture->GetTexture(), pixels);

        io.Fonts->TexID = (ImTextureID)mpFontTexture->GetSRV();

        RHI::RHIGraphicsPipelineStateDesc psoDesc;
        psoDesc.VS = mpRenderer->GetShader("ImGui.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        psoDesc.PS = mpRenderer->GetShader("ImGui.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        psoDesc.DepthStencilState.bDepthWrite = false;
        psoDesc.BlendState[0].bBlendEnable = true;
        psoDesc.BlendState[0].ColorSrc = RHI::ERHIBlendFactor::SrcAlpha;
        psoDesc.BlendState[0].ColorDst = RHI::ERHIBlendFactor::InvSrcAlpha;
        psoDesc.BlendState[0].AlphaSrc = RHI::ERHIBlendFactor::One;
        psoDesc.BlendState[0].AlphaDst = RHI::ERHIBlendFactor::InvSrcAlpha;
        psoDesc.RTFormats[0] = mpRenderer->GetSwapchain()->GetDesc()->ColorFormat;
        psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

        mpPSO = mpRenderer->GetPipelineState(psoDesc, "ImGuiPSO");

        return true;
    }

    void ImGuiImplement::NewFrame()
    {
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuizmo::BeginFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    }

    void ImGuiImplement::Render(RHI::RHICommandList *pCmdList)
    {
        GPU_EVENT_DEBUG(pCmdList, "ImGUI::Render");
        
        ImGui::Render();

        auto pDevice = mpRenderer->GetDevice();
        ImDrawData* drawData = ImGui::GetDrawData();

        if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        {
            return;
        }

        uint32_t frameIndex = pDevice->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;

        if (mpVertexBuffer[frameIndex] == nullptr || mpVertexBuffer[frameIndex]->GetBuffer()->GetDesc().Size < drawData->TotalVtxCount * sizeof(ImDrawVert))
        {
            mpVertexBuffer[frameIndex].reset(mpRenderer->CreateStructuredBuffer(nullptr, sizeof(ImDrawVert), drawData->TotalVtxCount + 5000, "GUI::VertexBuffer", RHI::ERHIMemoryType::CPUToGPU));
        }
        if (mpIndexBuffer[frameIndex] == nullptr || mpIndexBuffer[frameIndex]->GetIndexCount() < (uint32_t)drawData->TotalIdxCount)
        {
            mpIndexBuffer[frameIndex].reset(mpRenderer->CreateIndexBuffer(nullptr, sizeof(ImDrawIdx), drawData->TotalIdxCount + 10000, "GUI::IndexBuffer",  RHI::ERHIMemoryType::CPUToGPU));
        }

        ImDrawVert* vtxDst = (ImDrawVert*)mpVertexBuffer[frameIndex]->GetBuffer()->GetCPUAddress();
        ImDrawIdx* idxDst = (ImDrawIdx*)mpIndexBuffer[frameIndex]->GetBuffer()->GetCPUAddress();

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmdList->VtxBuffer.Size;
            idxDst += cmdList->IdxBuffer.Size;
        }
        SetupRenderStates(pCmdList, frameIndex);

        int globalVtxOffset = 0;
        int globalIdxOffset = 0;

        ImVec2 clipOff = drawData->DisplayPos;
        ImVec2 clipScale = drawData->FramebufferScale;
        uint32_t viewportWidth = drawData->DisplaySize.x * clipScale.x;
        uint32_t viewportHeight = drawData->DisplaySize.y * clipScale.y;

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            for (int i = 0; i < cmdList->CmdBuffer.size(); i++)
            {
                const ImDrawCmd* pCmd = &cmdList->CmdBuffer[i];
                if (pCmd->UserCallback != nullptr)
                {
                    if (pCmd->UserCallback == ImDrawCallback_ResetRenderState)
                    {
                        SetupRenderStates(pCmdList, frameIndex);
                    }
                    else
                    {
                        pCmd->UserCallback(cmdList, pCmd);
                    }
                }
                else
                {
                    ImVec2 clipMin((pCmd->ClipRect.x - clipOff.x) * clipScale.x, (pCmd->ClipRect.y - clipOff.y) * clipScale.y);
                    ImVec2 clipMax((pCmd->ClipRect.z - clipOff.x) * clipScale.x, (pCmd->ClipRect.w - clipOff.y) * clipScale.y);
                    if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                    {
                        continue;
                    }
                    pCmdList->SetScissorRect(
                        eastl::max(clipMin.x, 0.0f),
                        eastl::max(clipMin.y, 0.0f),
                        clamp((uint32_t)(clipMax.x - clipMin.x), 0u, viewportWidth),
                        clamp((uint32_t)(clipMax.y - clipMin.y), 0u, viewportHeight)
                    );

                    uint32_t resourceIds[4] = 
                    {
                        mpVertexBuffer[frameIndex]->GetSRV()->GetHeapIndex(),
                        pCmd->VtxOffset + globalVtxOffset,
                        ((RHI::RHIDescriptor*)pCmd->TextureId)->GetHeapIndex(),
                        mpRenderer->GetLinearSampler()->GetHeapIndex()
                    };
                    pCmdList->SetGraphicsConstants(0, resourceIds, sizeof(resourceIds));
                    pCmdList->DrawIndexed(pCmd->ElemCount, 1, pCmd->IdxOffset + globalIdxOffset);
                }
            }
            globalIdxOffset += cmdList->IdxBuffer.Size;
            globalVtxOffset += cmdList->VtxBuffer.Size;
        }
    }

    void ImGuiImplement::SetupRenderStates(RHI::RHICommandList *pCmdList, uint32_t frameIdx)
    {
        ImDrawData* drawData = ImGui::GetDrawData();

        pCmdList->SetViewport(0, 0, 
            (uint32_t)(drawData->DisplaySize.x * drawData->FramebufferScale.x),
            (uint32_t)(drawData->DisplaySize.y * drawData->FramebufferScale.y));
        pCmdList->SetPipelineState(mpPSO);
        pCmdList->SetIndexBuffer(mpIndexBuffer[frameIdx]->GetBuffer(), 0, mpIndexBuffer[frameIdx]->GetFormat());

        float L = drawData->DisplayPos.x;
        float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float T = drawData->DisplayPos.y;
        float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };

        pCmdList->SetGraphicsConstants(1, mvp, sizeof(mvp));
    }
}