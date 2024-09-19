#include "Camera.hpp"

#include "Core/VultanaEngine.hpp"
#include "Utilities/GUIUtil.hpp"
#include "Windows/GLFWindow.hpp"

namespace Scene
{
    Camera::Camera()
    {
        mPosition = { 0.0f, 0.0f, 0.0f };
        mRotation = { 0.0f, 0.0f, 0.0f };

        auto onResizeCallback = std::bind(&Camera::OnWindowResize, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        Core::VultanaEngine::GetEngineInstance()->GetWindowHandle()->OnResize(onResizeCallback);
    }

    Camera::~Camera()
    {
    }

    void Camera::SetPerspective(float aspectRatio, float yFov, float near)
    {
        mAspectRatio = aspectRatio;
        mFov = yFov;
        mNear = near;

        float h = 1.0 / std::tan(0.5f * Math::ToRadians(yFov));
        float w = h / aspectRatio;
        mProjection = Math::Matrix4x4(
            w,    0.0f,  0.0f,  0.0f,
            0.0f, h,     0.0f,  0.0f,
            0.0f, 0.0f,  0.0f,  1.0f,
            0.0f, 0.0f,  mNear, 0.0f
        );
    }

    void Camera::SetPosition(const Math::Vector3 &position)
    {
        mPosition = position;
    }

    void Camera::SetRotation(const Math::Vector3 &rotation)
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
        mbUpdated = false;

        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureKeyboard && !io.NavActive)
        {
            if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickLeft))
            {
                mPosition += GetLeft() * mMoveSpeed * deltaTime;
                mbUpdated = true;
            }

            if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickDown))
            {
                mPosition += GetBackward() * mMoveSpeed * deltaTime;
                mbUpdated = true;
            }

            if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight))
            {
                mPosition += GetRight() * mMoveSpeed * deltaTime;
                mbUpdated = true;
            }

            if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickUp))
            {
                mPosition += GetForward() * mMoveSpeed * deltaTime;
                mbUpdated = true;
            }
        }

        if (!io.WantCaptureMouse)
        {
            if (!glm::epsilonEqual(io.MouseWheel, 0.0f, 0.00001f))
            {
                mPosition += GetForward() * io.MouseWheel * mMoveSpeed * deltaTime;
                mbUpdated = true;
            }

            if (ImGui::IsMouseDragging(1))
            {
                const float rotateSpeed = 0.1f;

                mRotation.x = fmodf(mRotation.x + io.MouseDelta.y * rotateSpeed, 360.0f);
                mRotation.y = fmodf(mRotation.y + io.MouseDelta.x * rotateSpeed, 360.0f);
                mbUpdated = true;
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
    }

    void Camera::DrawViewFrustum(RHI::RHICommandList *pCmdList)
    {
    }

    void Camera::UpdateMatrix()
    {
        mWorld = Math::MakeTranslationMatrix(mPosition) * Math::MakeRotationMatrix(mRotation);
        mView = Math::Inverse(mWorld);
        mVPMat = mProjection * mView;
    }

    void Camera::OnWindowResize(Window::GLFWindow& wndHandle, uint32_t width, uint32_t height)
    {
        mAspectRatio = static_cast<float>(width) / static_cast<float>(height);
        SetPerspective(mAspectRatio, mFov, mNear);
    }
}
