#pragma once

#include "Utilities/Math.hpp"

namespace RHI
{
    class RHICommandList;
}

namespace Window
{
    class GLFWindow;
}

namespace Scene
{
    class Camera
    {
    public:
        Camera();
        ~Camera();

        void SetPerspective(float aspectRatio, float yFov, float near);
        void SetPosition(const Math::Vector3& position);
        void SetRotation(const Math::Vector3& rotation);
        void SetMoveSpeed(float speed) { mMoveSpeed = speed; }
        bool IsUpdated() const { return mbUpdated; }
        float GetZNear() const { return mNear; }
        void SetFOV(float fov);

        void Tick(float deltaTime);

        const Math::Vector3& GetPosition () const { return mPosition; }
        const Math::Vector3& GetRotation () const { return mRotation; }
        const Math::Matrix4x4& GetWorldMatrix() const { return mWorld; }
        const Math::Matrix4x4& GetViewMatrix() const { return mView; }
        const Math::Matrix4x4& GetProjectionMatrix() const { return mProjection; }
        const Math::Matrix4x4& GetViewProjectionMatrix() const { return mVPMat; }
        float GetMoveSpeed() const { return mMoveSpeed; }
        float GetFOV() const { return mFov; }

        const Math::Vector3 GetLeft() const { return Math::Vector3(-mWorld[0].x, -mWorld[0].y, -mWorld[0].z); }
        const Math::Vector3 GetRight() const { return Math::Vector3(mWorld[0].x, mWorld[0].y, mWorld[0].z); }
        const Math::Vector3 GetForward() const { return Math::Vector3(mWorld[2].x, mWorld[2].y, mWorld[2].z); }
        const Math::Vector3 GetBackward() const { return Math::Vector3(-mWorld[2].x, -mWorld[2].y, -mWorld[2].z); }
        const Math::Vector3 GetUp() const { return Math::Vector3(mWorld[1].x, mWorld[1].y, mWorld[1].z); }
        const Math::Vector3 GetDown() const { return Math::Vector3(-mWorld[1].x, -mWorld[1].y, -mWorld[1].z); }

        void DrawViewFrustum(RHI::RHICommandList* pCmdList);

    private:
        void UpdateMatrix();
        void OnWindowResize(Window::GLFWindow &wndHandle, uint32_t width, uint32_t height);

    private:
        Math::Vector3 mPosition = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 mRotation = { 0.0f, 0.0f, 0.0f };

        bool mbUpdated = false;

        Math::Matrix4x4 mWorld;
        Math::Matrix4x4 mView;
        Math::Matrix4x4 mProjection;
        Math::Matrix4x4 mVPMat;

        float mAspectRatio = 1.0f;
        float mFov = 45.0f;
        float mNear = 0.1f;
        float mFar = 1000.0f;

        float mMoveSpeed = 10.0f;
    };
}