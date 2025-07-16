#include "Camera.hpp"

#include "Core/VultanaEngine.hpp"
#include "Utilities/GUIUtil.hpp"
#include "Utilities/Log.hpp"
#include "Window/GLFWindow.hpp"

namespace Scene
{
    static inline float2 CalcDepthLinearParams(const float4x4& mtxProjection)
    {
        // | 1  0  0  0 |
        // | 0  1  0  0 |
        // | 0  0  A  1 |
        // | 0  0  B  0 |

        // Z' = (Z * A + B) / Z     --- perspective divide
        // Z' = A + B / Z 

        // Z = B / (Z' - A)
        // Z = 1 / (Z' * C1 - C2)   --- C1 = 1/B, C2 = A/B
        float A = mtxProjection[2][2];
        float B = max(mtxProjection[3][2], 0.00000001f);
        float C1 = 1.0f / B;
        float C2 = A / B;
        
        return float2(C1, C2);
    }

    Camera::Camera()
    {
        m_Position = { 0.0f, 0.0f, 0.0f };
        m_Rotation = { 0.0f, 0.0f, 0.0f };

        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.connect(&Camera::OnWindowResize, this);
    }

    Camera::~Camera()
    {
        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.disconnect(this);
    }

    void Camera::SetPerspective(float aspectRatio, float yFov, float zNear)
    {
        m_AspectRatio = aspectRatio;
        m_Fov = yFov;
        m_Near = zNear;

        float h = 1.0 / tan(0.5f * DegreeToRadian(yFov));
        float w = h / aspectRatio;
        m_Projection = float4x4(0.0);
        m_Projection[0][0] = w;
        m_Projection[1][1] = h;
        m_Projection[2][2] = 0.0f;
        m_Projection[2][3] = 1.0f;
        m_Projection[3][2] = m_Near;
    }

    void Camera::SetPosition(const float3 &position)
    {
        m_Position = position;
    }

    void Camera::SetRotation(const float3 &rotation)
    {
        m_Rotation = rotation;
    }

    void Camera::SetFOV(float fov)
    {
        m_Fov = fov;
        SetPerspective(m_AspectRatio, m_Fov, m_Near);
    }

    void Camera::Tick(float deltaTime)
    {
        GUICommand("Settings", "Camera", [&]() { OnCameraSettingGUI(); });

        m_bMoved = false;

        UpdateCameraPosition(deltaTime);
        UpdateCameraRotation(deltaTime);

        UpdateMatrix();

        if (!m_bFrustumLocked)
        {
            m_FrustumViewPos = m_Position;
            // No jitter, use original vp matrix
            UpdateFrustumPlanes(m_ViewProjMat);
        }
    }

    void Camera::SetupCameraCB(FCameraConstants &cameraCB)
    {
        cameraCB.CameraPosition = GetPosition();
        cameraCB.NearPlane = m_Near;

        cameraCB.MtxView = m_View;
        cameraCB.MtxViewInverse = Inverse(m_View);
        cameraCB.MtxProjection = m_Projection;
        cameraCB.MtxProjectionInverse = Inverse(m_Projection);
        cameraCB.MtxViewProjection = m_ViewProjMat;
        cameraCB.MtxViewProjectionInverse = Inverse(m_ViewProjMat);
    }

    void Camera::DrawViewFrustum(RHI::RHICommandList *pCmdList)
    {
    }

    void Camera::UpdateFrustumPlanes(const float4x4 &matrix)
    {
        // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

        // Left clipping plane
        m_FrustumPlanes[0].x = matrix[0][3] + matrix[0][0];
        m_FrustumPlanes[0].y = matrix[1][3] + matrix[1][0];
        m_FrustumPlanes[0].z = matrix[2][3] + matrix[2][0];
        m_FrustumPlanes[0].w = matrix[3][3] + matrix[3][0];
        m_FrustumPlanes[0] = NormalizePlane(m_FrustumPlanes[0]);

        // Right clipping plane
        m_FrustumPlanes[1].x = matrix[0][3] - matrix[0][0];
        m_FrustumPlanes[1].y = matrix[1][3] - matrix[1][0];
        m_FrustumPlanes[1].z = matrix[2][3] - matrix[2][0];
        m_FrustumPlanes[1].w = matrix[3][3] - matrix[3][0];
        m_FrustumPlanes[1] = NormalizePlane(m_FrustumPlanes[1]);

        // Top clipping plane
        m_FrustumPlanes[2].x = matrix[0][3] - matrix[0][1];
        m_FrustumPlanes[2].y = matrix[1][3] - matrix[1][1];
        m_FrustumPlanes[2].z = matrix[2][3] - matrix[2][1];
        m_FrustumPlanes[2].w = matrix[3][3] - matrix[3][1];
        m_FrustumPlanes[2] = NormalizePlane(m_FrustumPlanes[2]);

        // Bottom clipping plane
        m_FrustumPlanes[3].x = matrix[0][3] + matrix[0][1];
        m_FrustumPlanes[3].y = matrix[1][3] + matrix[1][1];
        m_FrustumPlanes[3].z = matrix[2][3] + matrix[2][1];
        m_FrustumPlanes[3].w = matrix[3][3] + matrix[3][1];
        m_FrustumPlanes[3] = NormalizePlane(m_FrustumPlanes[3]);

        // far clipping plane (reversed depth)
        m_FrustumPlanes[4].x = matrix[0][2];
        m_FrustumPlanes[4].y = matrix[1][2];
        m_FrustumPlanes[4].z = matrix[2][2];
        m_FrustumPlanes[4].w = matrix[3][2];
        m_FrustumPlanes[4] = NormalizePlane(m_FrustumPlanes[4]);

        // near clipping plane (reversed depth)
        m_FrustumPlanes[5].x = matrix[0][3] - matrix[0][2];
        m_FrustumPlanes[5].y = matrix[1][3] - matrix[1][2];
        m_FrustumPlanes[5].z = matrix[2][3] - matrix[2][2];
        m_FrustumPlanes[5].w = matrix[3][3] - matrix[3][2];
        m_FrustumPlanes[5] = NormalizePlane(m_FrustumPlanes[5]);
    }

    void Camera::UpdateMatrix()
    {
        m_World = mul(translation_matrix(m_Position), rotation_matrix(RotationQuat(m_Rotation)));
        m_View = Inverse(m_World);
        m_ViewProjMat = mul(m_Projection, m_View);
    }

    void Camera::OnWindowResize(void* wndHandle, uint32_t width, uint32_t height)
    {
        m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
        SetPerspective(m_AspectRatio, m_Fov, m_Near);
    }

    void Camera::OnCameraSettingGUI()
    {
        bool perspectiveUpdated = false;
        perspectiveUpdated |= ImGui::SliderFloat("FOV", &m_Fov, 5.0f, 135.0f, "%.0f");
        perspectiveUpdated |= ImGui::SliderFloat("Near Plane", &m_Near, 0.0001f, 3.0f, "%.4f");

        if (perspectiveUpdated)
        {
            SetPerspective(m_AspectRatio, m_Fov, m_Near);
        }

        ImGui::SliderFloat("Movement Speed", &m_MoveSpeed, 1.0f, 200.0f, "%.0f");
        ImGui::SliderFloat("Rotation Speed", &m_RotateSpeed, 0.1f, 10.0f, "%.1f");
        ImGui::SliderFloat("Movement Smoothness", &m_MoveSmoothness, 0.01f, 1.0f, "%.2f");
        ImGui::SliderFloat("Rotation Smoothness", &m_RotateSmoothness, 0.01f, 1.0f, "%.2f");
    }

    void Camera::UpdateCameraPosition(float deltaTime)
    {
        bool moveLeft = false;
        bool moveRight = false;
        bool moveForward = false;
        bool moveBackward = false;
        float moveSpeed = m_MoveSpeed;

        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureKeyboard && !io.NavActive && !io.WantCaptureMouse)
        {
            bool shouldMove = ImGui::IsMouseDragging(1);
            bool shouldSpeedUp = shouldMove && ImGui::IsKeyDown(ImGuiKey_LeftShift) && 
                (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_D));
            const float speedUp = shouldSpeedUp ? 2.0f : 1.0f;
            moveSpeed *= speedUp;

            moveLeft = (ImGui::IsKeyDown(ImGuiKey_A) && shouldMove) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickLeft);
            moveRight = (ImGui::IsKeyDown(ImGuiKey_D) && shouldMove) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight);
            moveForward = (ImGui::IsKeyDown(ImGuiKey_W) && shouldMove) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickUp);
            moveBackward = (ImGui::IsKeyDown(ImGuiKey_S) && shouldMove) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickDown);
        }
        if (!io.WantCaptureMouse)
        {
            if (!NearlyEqual(io.MouseWheel, 0.0f))
            {
                moveForward |= io.MouseWheel > 0.0f;
                moveBackward |= io.MouseWheel < 0.0f;

                moveSpeed *= abs(io.MouseWheel) * m_MoveSpeed;
            }
            if (ImGui::IsMouseDragging(1) && ImGui::IsKeyDown(ImGuiKey_LeftAlt))
            {
                moveForward |= io.MouseDelta.y > 0.0f;
                moveBackward |= io.MouseDelta.y < 0.0f;

                moveSpeed *= abs(io.MouseDelta.y);
            }
        }

        float3 moveVelocity = float3(0, 0, 0);
        if (moveForward) moveVelocity += GetForward();
        if (moveBackward) moveVelocity += GetBackward();
        if (moveLeft) moveVelocity += GetLeft();
        if (moveRight) moveVelocity += GetRight();

        if (length(moveVelocity) > 0.0f)
        {
            moveVelocity = normalize(moveVelocity) * moveSpeed;
        }

        moveVelocity = lerp(m_PrevMoveVelocity, moveVelocity, 1.0f - exp(-deltaTime * 10.0f / m_MoveSmoothness));
        m_PrevMoveVelocity = moveVelocity;

        m_Position += moveVelocity * deltaTime;
        m_World = mul(translation_matrix(m_Position), rotation_matrix(RotationQuat(m_Rotation)));

        m_bMoved |= length(moveVelocity * deltaTime) > 0.0001f;
    }

    void Camera::UpdateCameraRotation(float deltaTime)
    {
        ImGuiIO& io = ImGui::GetIO();

        float2 rotateVelocity = {};

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

        m_Rotation.x = NormalizeAngle(m_Rotation.x + rotateVelocity.x * deltaTime * 100.0f);
        m_Rotation.y = NormalizeAngle(m_Rotation.y + rotateVelocity.y * deltaTime * 100.0f);
        m_World = mul(translation_matrix(m_Position), rotation_matrix(RotationQuat(m_Rotation)));

        m_bMoved |= length(rotateVelocity * deltaTime) > 0.0001f;
    }
}
