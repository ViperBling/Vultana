#include "VultanaGUI.hpp"

#include "VultanaEngine.hpp"

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGuizmo.h>

namespace Core
{
    GUI::GUI()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

        io.BackendRendererName = "imgui_impl_vultana";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(Core::VultanaEngine::GetEngineInstance()->GetWindowHandle());
    }

    GUI::~GUI()
    {
        ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();
    }

    bool GUI::Init()
    {
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto pDevice = pRenderer->GetDevice();

        float scaling = ImGui_ImplWin32_GetDpiScaleForHwnd(Core::VultanaEngine::GetEngineInstance()->GetWindowHandle());
        ImGui::GetStyle().ScaleAllSizes(scaling);

        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = scaling;

        ImFontConfig fontConfig;
        fontConfig.OversampleH = fontConfig.OversampleV = 3;

        std::string fontFile = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath() + "Fonts/Roboto-Medium.ttf";
        io.Fonts->AddFontFromFileTTF(fontFile.c_str(), 13.0f, &fontConfig);

        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        mpFontTexture.reset(pRenderer->CreateTexture2D(width, height, 1, RHI::ERHIFormat::RGBA8UNORM, 0, "GUI::FontTexture"));
        pRenderer->UploadTexture(mpFontTexture->GetTexture(), pixels);

        io.Fonts->TexID = (ImTextureID)mpFontTexture->GetSRV();

        RHI::RHIGraphicsPipelineStateDesc psoDesc;
        psoDesc.VS = pRenderer->GetShader("ImGui.hlsl", "VSMain", RHI::ERHIShaderType::VS);
        psoDesc.PS = pRenderer->GetShader("ImGui.hlsl", "PSMain", RHI::ERHIShaderType::PS);
        psoDesc.DepthStencilState.bDepthWrite = false;
        psoDesc.BlendState[0].bBlendEnable = true;
        psoDesc.BlendState[0].ColorSrc = RHI::ERHIBlendFactor::SrcAlpha;
        psoDesc.BlendState[0].ColorDst = RHI::ERHIBlendFactor::InvSrcAlpha;
        psoDesc.BlendState[0].AlphaSrc = RHI::ERHIBlendFactor::One;
        psoDesc.BlendState[0].AlphaDst = RHI::ERHIBlendFactor::InvSrcAlpha;
        psoDesc.RTFormats[0] = pRenderer->GetSwapchain()->GetDesc()->ColorFormat;
        psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

        mpPSO = pRenderer->GetPipelineState(psoDesc, "ImGuiPSO");

        return true;
    }

    void GUI::Tick()
    {
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuizmo::BeginFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    }

    void GUI::Render(RHI::RHICommandList *pCmdList)
    {
        GPU_EVENT_DEBUG(pCmdList, "GUI::Render");
        for (size_t i = 0; i < mCommands.size(); i++)
        {
            mCommands[i]();
        }
        mCommands.clear();

        ImGui::Render();

        // TODO
    }

    void GUI::SetupRenderStates(RHI::RHICommandList *pCmdList, uint32_t frameIdx)
    {
        // ImDrawData* drawData = ImGui::GetDrawData();

        // pCmdList->SetViewport(0, 0, 
        //     (uin32_t)(drawData->DisplaySize.x * drawData->FramebufferScale.x),
        //     (uin32_t)(drawData->DisplaySize.y * drawData->FramebufferScale.y));
        // pCmdList->SetPipelineState(mpPSO);
        // pCmdList->SetIndexBuffer(mpIndexBuffer[frameIdx]->GetBuffer(), 0, mpIndexBuffer[frameIdx]->GetFormat());

        // TODO
    }
}