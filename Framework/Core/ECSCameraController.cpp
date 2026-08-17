#include "Core/ECSCameraController.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/GUIUtil.hpp"

#include <imgui.h>

namespace Core
{
    void FCameraController::Tick(float deltaTime, Scene::ECS::FEntitySubsystem& subsystem)
    {
        GUICommand("Settings", "Camera", [this]() { OnSettingsGUI(); });

        auto& reg = subsystem.GetRegistry();
        auto cameraView = reg.view<Scene::ECS::FTransformComponent, Scene::ECS::FCameraComponent, Scene::ECS::FMainCameraTag>();
        const entt::entity cameraEntity = cameraView.front();
        if (cameraEntity == entt::null || !reg.valid(cameraEntity)) return;

        auto* tf = reg.try_get<Scene::ECS::FTransformComponent>(cameraEntity);
        auto* cam = reg.try_get<Scene::ECS::FCameraComponent>(cameraEntity);
        if (tf == nullptr || cam == nullptr) return;

        if (m_CameraEntity != cameraEntity)
        {
            m_CameraEntity = cameraEntity;
            m_State.Fov = cam->Fov;
            m_State.NearPlane = cam->ZNear;
            m_State.FarPlane = cam->ZFar;
            m_State.Position = tf->Position;
            m_State.Rotation = RotationAngles(tf->Rotation);
            m_LastWrittenPosition = tf->Position;
            m_LastWrittenRotation = tf->Rotation;
        }
        else
        {
            if (m_LastWrittenPosition != tf->Position || m_LastWrittenRotation != tf->Rotation)
            {
                m_State.Position = tf->Position;
                m_State.Rotation = RotationAngles(tf->Rotation);
            }
        }
        cam->Fov = m_State.Fov;
        cam->ZNear = m_State.NearPlane;

        float3 position = m_State.Position;
        float3 rotation = m_State.Rotation;
        const float4x4 previousWorld = mul(translation_matrix(position), rotation_matrix(RotationQuat(rotation)));

        UpdatePosition(deltaTime, previousWorld, position);
        UpdateRotation(deltaTime, rotation);

        m_State.Position = position;
        m_State.Rotation = rotation;
        m_State.Fov = cam->Fov;
        m_State.NearPlane = cam->ZNear;
        m_State.FarPlane = cam->ZFar;
        Renderer::UpdateCameraMathState(m_State);

        const float4 newRotation = RotationQuat(rotation);
        if (tf->Position != position || tf->Rotation != newRotation)
        {
            tf->Position = position;
            tf->Rotation = newRotation;
            m_LastWrittenPosition = position;
            m_LastWrittenRotation = newRotation;
            subsystem.MarkTransformDirty(cameraEntity);
        }
    }

    void FCameraController::OnSettingsGUI()
    {
        float fov = m_State.Fov;
        float nearPlane = m_State.NearPlane;
        bool bPerspectiveUpdated = false;
        bPerspectiveUpdated |= ImGui::SliderFloat("FOV", &fov, 5.0f, 135.0f, "%.0f");
        bPerspectiveUpdated |= ImGui::SliderFloat("Near Plane", &nearPlane, 0.0001f, 3.0f, "%.4f");
        if (bPerspectiveUpdated)
        {
            m_State.Fov = fov;
            m_State.NearPlane = nearPlane;
        }
        ImGui::SliderFloat("Movement Speed", &m_MoveSpeed, 1.0f, 200.0f, "%.0f");
        ImGui::SliderFloat("Rotation Speed", &m_RotateSpeed, 0.1f, 10.0f, "%.1f");
        ImGui::SliderFloat("Movement Smoothness", &m_MoveSmoothness, 0.01f, 1.0f, "%.2f");
        ImGui::SliderFloat("Rotation Smoothness", &m_RotateSmoothness, 0.01f, 1.0f, "%.2f");
    }

    void FCameraController::UpdatePosition(float deltaTime, const float4x4& world, float3& position)
    {
        ImGuiIO& io = ImGui::GetIO();
        bool moveLeft = false, moveRight = false, moveForward = false, moveBackward = false, moveUp = false, moveDown = false;
        float moveSpeed = m_MoveSpeed;

        if (!io.WantCaptureKeyboard && !io.NavActive && !io.WantCaptureMouse)
        {
            const bool shouldMove = ImGui::IsMouseDragging(1);
            const bool shouldSpeedUp = shouldMove && ImGui::IsKeyDown(ImGuiKey_LeftShift) && (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_D));

            moveSpeed *= shouldSpeedUp ? 2.0f : 1.0f;
            moveLeft = (ImGui::IsKeyDown(ImGuiKey_A) && shouldMove) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickLeft);
            moveRight = (ImGui::IsKeyDown(ImGuiKey_D) && shouldMove) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight);
            moveForward = (ImGui::IsKeyDown(ImGuiKey_W) && shouldMove) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickUp);
            moveBackward = (ImGui::IsKeyDown(ImGuiKey_S) && shouldMove) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickDown);
            moveUp = (ImGui::IsKeyDown(ImGuiKey_E) && shouldMove && ImGui::IsKeyDown(ImGuiKey_LeftShift));
            moveDown = (ImGui::IsKeyDown(ImGuiKey_Q) && shouldMove && ImGui::IsKeyDown(ImGuiKey_LeftShift));
        }
        if (!io.WantCaptureMouse)
        {
            if (!NearlyEqual(io.MouseWheel, 0.0f))
            {
                moveForward |= io.MouseWheel > 0.0f;
                moveBackward |= io.MouseWheel < 0.0f;
                moveSpeed *= Abs(io.MouseWheel) * m_MoveSpeed;
            }
            if (ImGui::IsMouseDragging(1) && ImGui::IsKeyDown(ImGuiKey_LeftAlt))
            {
                moveForward |= io.MouseDelta.y > 0.0f;
                moveBackward |= io.MouseDelta.y < 0.0f;
                moveSpeed *= Abs(io.MouseDelta.y);
            }
        }

        const float3 forward = world[2].xyz();
        const float3 backward = -forward;
        const float3 left = -world[0].xyz();
        const float3 right = world[0].xyz();
        const float3 up = float3(0.0f, 1.0f, 0.0f);   // world-space vertical (Q down / E up)

        float3 moveVelocity = float3(0.0f);
        if (moveForward) moveVelocity += forward;
        if (moveBackward) moveVelocity += backward;
        if (moveLeft) moveVelocity += left;
        if (moveRight) moveVelocity += right;
        if (moveUp) moveVelocity += up;
        if (moveDown) moveVelocity -= up;
        if (length(moveVelocity) > 0.0f)
            moveVelocity = normalize(moveVelocity) * moveSpeed;

        moveVelocity = lerp(m_PrevMoveVelocity, moveVelocity, 1.0f - exp(-deltaTime * 10.0f / m_MoveSmoothness));
        m_PrevMoveVelocity = moveVelocity;
        position += moveVelocity * deltaTime;
    }

    void FCameraController::UpdateRotation(float deltaTime, float3& rotation)
    {
        ImGuiIO& io = ImGui::GetIO();
        float2 rotateVelocity = float2(0.0f);
        if (!io.WantCaptureMouse)
        {
            if (ImGui::IsMouseDragging(1) && !ImGui::IsKeyDown(ImGuiKey_LeftAlt))
            {
                rotateVelocity.x = io.MouseDelta.y * m_RotateSpeed;
                rotateVelocity.y = io.MouseDelta.x * m_RotateSpeed;
            }
        }
        rotateVelocity = lerp(m_PrevRotateVelocity, rotateVelocity, 1.0f - exp(-deltaTime * 10.0f / m_RotateSmoothness));
        m_PrevRotateVelocity = rotateVelocity;
        rotation.x = NormalizeAngle(rotation.x + rotateVelocity.x * deltaTime * 100.0f);
        rotation.y = NormalizeAngle(rotation.y + rotateVelocity.y * deltaTime * 100.0f);
    }
}