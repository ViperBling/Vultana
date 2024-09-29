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

        float h = 1.0 / std::tan(0.5f * DegreeToRadian(yFov));
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
        // auto wndHandle = Core::VultanaEngine::GetEngineInstance()->GetWindowHandle();

        GUI("Settings", "Camera", [&]() { OnCameraSettingGUI(); });
        mbMoved = false;

        // if (wndHandle->IsMousePressed(Utility::MouseButton::RIGHT))
        // {
        //     auto cursorDelta = wndHandle->GetCursorDelta();

        //     mRotation.x = fmodf(mRotation.x + cursorDelta.y * mRotateSpeed, 360.0f);
        //     mRotation.y = fmodf(mRotation.y + cursorDelta.x * mRotateSpeed, 360.0f);

        //     mbMoved = true;
        // }
        // if (wndHandle->IsMouseReleased(Utility::MouseButton::RIGHT))
        // {
        //     wndHandle->SetLastCursorPosition(wndHandle->GetCursorPosition());
        // }

        // if (wndHandle->IsKeyPressed(Utility::KeyCode::W))
        // {
        //     mPosition += GetForward() * mMoveSpeed * deltaTime;
        //     mbMoved = true;
        // }
        // if (wndHandle->IsKeyPressed(Utility::KeyCode::S))
        // {
        //     mPosition += GetBackward() * mMoveSpeed * deltaTime;
        //     mbMoved = true;
        // }
        // if (wndHandle->IsKeyPressed(Utility::KeyCode::A))
        // {
        //     mPosition += GetLeft() * mMoveSpeed * deltaTime;
        //     mbMoved = true;
        // }
        // if (wndHandle->IsKeyPressed(Utility::KeyCode::D))
        // {
        //     mPosition += GetRight() * mMoveSpeed * deltaTime;
        //     mbMoved = true;
        // }

        // ==== Temporarily disable imgui control ====
        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureKeyboard && !io.NavActive)
        {
            if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickLeft))
            {
                mPosition += GetLeft() * mMoveSpeed * deltaTime;
                mbMoved = true;
            }

            if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickDown))
            {
                mPosition += GetBackward() * mMoveSpeed * deltaTime;
                mbMoved = true;
            }

            if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight))
            {
                mPosition += GetRight() * mMoveSpeed * deltaTime;
                mbMoved = true;
            }

            if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickUp))
            {
                mPosition += GetForward() * mMoveSpeed * deltaTime;
                mbMoved = true;
            }
        }

        if (!io.WantCaptureMouse)
        {
            if (!NearlyEqual(io.MouseWheel, 0.0f))
            {
                mPosition += GetForward() * io.MouseWheel * mMoveSpeed * deltaTime;
                mbMoved = true;
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
            {
                const float rotateSpeed = 0.1f;

                mRotation.x = fmodf(mRotation.x + io.MouseDelta.y * rotateSpeed, 360.0f);
                mRotation.y = fmodf(mRotation.y + io.MouseDelta.x * rotateSpeed, 360.0f);
                mbMoved = true;
            }
        }

        if (!io.NavActive)
        {
            const float rotateSpeed = 120.0f;

            if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickRight))
            {
                mRotation.y = fmodf(mRotation.y + deltaTime * rotateSpeed, 360.0f);
            }

            if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickLeft))
            {
                mRotation.y = fmodf(mRotation.y - deltaTime * rotateSpeed, 360.0f);
            }

            if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickDown))
            {
                mRotation.x = fmodf(mRotation.x + deltaTime * rotateSpeed, 360.0f);
            }

            if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickUp))
            {
                mRotation.x = fmodf(mRotation.x - deltaTime * rotateSpeed, 360.0f);
            }
        }

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
        ImGui::SliderFloat("Move Speed", &mMoveSpeed, 1.0f, 200.0f, "%.0f");

        bool perspectiveUpdated = false;
        perspectiveUpdated |= ImGui::SliderFloat("FOV", &mFov, 5.0f, 135.0f, "%.0f");
        perspectiveUpdated |= ImGui::SliderFloat("Near", &mNear, 0.0001f, 3.0f, "%.4f");

        if (perspectiveUpdated)
        {
            SetPerspective(mAspectRatio, mFov, mNear);
        }
    }
}
