#include "DirectionalLight.hpp"
#include "Utilities/GUIUtil.hpp"

namespace Scene
{
    bool DirectionalLight::Create()
    {
        return true;
    }

    void DirectionalLight::Tick(float deltaTime)
    {
        float4x4 R = rotation_matrix(m_Rotation);
        m_LightDirection = normalize(mul(R, float4(0.0f, 1.0f, 0.0f, 0.0f)).xyz());

        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard && !io.WantCaptureMouse && ImGui::IsKeyDown(ImGuiKey_L))
        {
            Camera* camera = Core::VultanaEngine::GetEngineInstance()->GetWorld()->GetCamera();
            const float4x4& mtxView = camera->GetViewMatrix();

            m_LightDirection = normalize(mul(mtxView, float4(m_LightDirection, 0.0f))).xyz();

            static float3 rotation = {};
            rotation.z += io.MouseDelta.x * 0.1f;
            rotation.x += io.MouseDelta.y * 0.1f;

            float4x4 viewSpaceR = rotation_matrix(RotationQuat(rotation));
            m_LightDirection = mul(viewSpaceR, float4(m_LightDirection, 0.0f)).xyz();
            m_LightDirection = normalize(mul(inverse(mtxView), float4(m_LightDirection, 0.0f))).xyz();
        }

        GUICommand("Settings", "Directional Light", [&]()
        {
            ImGui::DragFloat3("Rotation##Directional Light", &m_Rotation.x, 1.0f, -180.0f, 180.0f);
            ImGui::DragFloat("Radius##DirectionalLight", &m_LightRadius, 0.01f, 0.0f, 0.1f);
            ImGui::DragFloat("Intensity##DirectionalLight", &m_LightIntensity, 0.01f, 0.0f, 100.0f);
            ImGui::ColorEdit3("Color##DirectionalLight", &m_LightColor.x);
        });
    }
}