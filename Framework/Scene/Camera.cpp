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
        mPosition = { 0.0f, 0.0f, 0.0f };
        mRotation = { 0.0f, 0.0f, 0.0f };

        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.connect(&Camera::OnWindowResize, this);
    }

    Camera::~Camera()
    {
        Core::VultanaEngine::GetEngineInstance()->OnWindowResizeSignal.disconnect(this);
    }

    void Camera::SetPerspective(float aspectRatio, float yFov, float zNear)
    {
        mAspectRatio = aspectRatio;
        mFov = yFov;
        mNear = zNear;

        float h = 1.0 / tan(0.5f * DegreeToRadian(yFov));
        float w = h / aspectRatio;
        mProjection = float4x4(0.0);
        mProjection[0][0] = w;
        mProjection[1][1] = h;
        mProjection[2][2] = 0.0f;
        mProjection[2][3] = 1.0f;
        mProjection[3][2] = mNear;
    }

    void Camera::SetPosition(const float3 &position)
    {
        mPosition = position;
    }

    void Camera::SetRotation(const float3 &rotation)
    {
        mRotation = rotation;
    }

    void Camera::SetFOV(float fov)
    {
        mFov = fov;
        SetPerspective(mAspectRatio, mFov, mNear);
    }

    void Camera::Tick(float deltaTime)
    {
        GUICommand("Settings", "Camera", [&]() { OnCameraSettingGUI(); });

        mbMoved = false;

        UpdateCameraPosition(deltaTime);
        UpdateCameraRotation(deltaTime);

        UpdateMatrix();

        if (!mbFrustumLocked)
        {
            mFrustumViewPos = mPosition;
            // No jitter, use original vp matrix
            UpdateFrustumPlanes(mViewProjMat);
        }
    }

    void Camera::SetupCameraCB(FCameraConstants &cameraCB)
    {
        cameraCB.CameraPosition = GetPosition();
        cameraCB.NearPlane = mNear;

        cameraCB.MtxView = mView;
        cameraCB.MtxViewInverse = Inverse(mView);
        cameraCB.MtxProjection = mProjection;
        cameraCB.MtxProjectionInverse = Inverse(mProjection);
        cameraCB.MtxViewProjection = mViewProjMat;
        cameraCB.MtxViewProjectionInverse = Inverse(mViewProjMat);
    }

    void Camera::DrawViewFrustum(RHI::RHICommandList *pCmdList)
    {
    }

    void Camera::UpdateFrustumPlanes(const float4x4 &matrix)
    {
        // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

        // Left clipping plane
        mFrustumPlanes[0].x = matrix[0][3] + matrix[0][0];
        mFrustumPlanes[0].y = matrix[1][3] + matrix[1][0];
        mFrustumPlanes[0].z = matrix[2][3] + matrix[2][0];
        mFrustumPlanes[0].w = matrix[3][3] + matrix[3][0];
        mFrustumPlanes[0] = NormalizePlane(mFrustumPlanes[0]);

        // Right clipping plane
        mFrustumPlanes[1].x = matrix[0][3] - matrix[0][0];
        mFrustumPlanes[1].y = matrix[1][3] - matrix[1][0];
        mFrustumPlanes[1].z = matrix[2][3] - matrix[2][0];
        mFrustumPlanes[1].w = matrix[3][3] - matrix[3][0];
        mFrustumPlanes[1] = NormalizePlane(mFrustumPlanes[1]);

        // Top clipping plane
        mFrustumPlanes[2].x = matrix[0][3] - matrix[0][1];
        mFrustumPlanes[2].y = matrix[1][3] - matrix[1][1];
        mFrustumPlanes[2].z = matrix[2][3] - matrix[2][1];
        mFrustumPlanes[2].w = matrix[3][3] - matrix[3][1];
        mFrustumPlanes[2] = NormalizePlane(mFrustumPlanes[2]);

        // Bottom clipping plane
        mFrustumPlanes[3].x = matrix[0][3] + matrix[0][1];
        mFrustumPlanes[3].y = matrix[1][3] + matrix[1][1];
        mFrustumPlanes[3].z = matrix[2][3] + matrix[2][1];
        mFrustumPlanes[3].w = matrix[3][3] + matrix[3][1];
        mFrustumPlanes[3] = NormalizePlane(mFrustumPlanes[3]);

        // far clipping plane (reversed depth)
        mFrustumPlanes[4].x = matrix[0][2];
        mFrustumPlanes[4].y = matrix[1][2];
        mFrustumPlanes[4].z = matrix[2][2];
        mFrustumPlanes[4].w = matrix[3][2];
        mFrustumPlanes[4] = NormalizePlane(mFrustumPlanes[4]);

        // near clipping plane (reversed depth)
        mFrustumPlanes[5].x = matrix[0][3] - matrix[0][2];
        mFrustumPlanes[5].y = matrix[1][3] - matrix[1][2];
        mFrustumPlanes[5].z = matrix[2][3] - matrix[2][2];
        mFrustumPlanes[5].w = matrix[3][3] - matrix[3][2];
        mFrustumPlanes[5] = NormalizePlane(mFrustumPlanes[5]);
    }

    void Camera::UpdateMatrix()
    {
        mWorld = mul(translation_matrix(mPosition), rotation_matrix(RotationQuat(mRotation)));
        mView = Inverse(mWorld);
        mViewProjMat = mul(mProjection, mView);
    }

    void Camera::OnWindowResize(void* wndHandle, uint32_t width, uint32_t height)
    {
        mAspectRatio = static_cast<float>(width) / static_cast<float>(height);
        SetPerspective(mAspectRatio, mFov, mNear);
    }

    void Camera::OnCameraSettingGUI()
    {
        bool perspectiveUpdated = false;
        perspectiveUpdated |= ImGui::SliderFloat("FOV", &mFov, 5.0f, 135.0f, "%.0f");
        perspectiveUpdated |= ImGui::SliderFloat("Near Plane", &mNear, 0.0001f, 3.0f, "%.4f");

        if (perspectiveUpdated)
        {
            SetPerspective(mAspectRatio, mFov, mNear);
        }

        ImGui::SliderFloat("Movement Speed", &mMoveSpeed, 1.0f, 200.0f, "%.0f");
        ImGui::SliderFloat("Rotation Speed", &mRotateSpeed, 0.1f, 10.0f, "%.1f");
        ImGui::SliderFloat("Movement Smoothness", &mMoveSmoothness, 0.01f, 1.0f, "%.2f");
        ImGui::SliderFloat("Rotation Smoothness", &mRotateSmoothness, 0.01f, 1.0f, "%.2f");
    }

    void Camera::UpdateCameraPosition(float deltaTime)
    {
        bool moveLeft = false;
        bool moveRight = false;
        bool moveForward = false;
        bool moveBackward = false;
        float moveSpeed = mMoveSpeed;

        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureKeyboard && !io.NavActive && !io.WantCaptureMouse)
        {
            bool shouldMove = ImGui::IsMouseDragging(1);
            const float speedUp = (ImGui::IsKeyDown(ImGuiKey_LeftShift) && shouldMove) ? 2.0f : 1.0f;
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

                moveSpeed *= abs(io.MouseWheel) * mMoveSpeed;
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
            moveVelocity = normalize(moveVelocity) * mMoveSpeed;
        }

        moveVelocity = lerp(mPrevMoveVelocity, moveVelocity, 1.0f - exp(-deltaTime * 10.0f / mMoveSmoothness));
        mPrevMoveVelocity = moveVelocity;

        mPosition += moveVelocity * deltaTime;
        mWorld = mul(translation_matrix(mPosition), rotation_matrix(RotationQuat(mRotation)));

        mbMoved |= length(moveVelocity * deltaTime) > 0.0001f;
    }

    void Camera::UpdateCameraRotation(float deltaTime)
    {
        ImGuiIO& io = ImGui::GetIO();

        float2 rotateVelocity = {};

        if (!io.WantCaptureMouse)
        {
            if (ImGui::IsMouseDragging(1) && !ImGui::IsKeyDown(ImGuiKey_LeftAlt))
            {
                rotateVelocity.x = io.MouseDelta.y * mRotateSpeed;
                rotateVelocity.y = io.MouseDelta.x * mRotateSpeed;
            }
        }
        rotateVelocity = lerp(mPrevRotateVelocity, rotateVelocity, 1.0f - exp(-deltaTime * 10.0f / mRotateSmoothness));
        mPrevRotateVelocity = rotateVelocity;

        mRotation.x += rotateVelocity.x * deltaTime * 100.0f;
        mRotation.y += rotateVelocity.y * deltaTime * 100.0f;
        mWorld = mul(translation_matrix(mPosition), rotation_matrix(RotationQuat(mRotation)));

        mbMoved |= length(rotateVelocity * deltaTime) > 0.0001f;
    }
}
