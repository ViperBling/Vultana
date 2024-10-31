#pragma once

#include "Utilities/Math.hpp"
#include "Common/GlobalConstants.hlsli"

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
        void SetPosition(const float3& position);
        void SetRotation(const float3& rotation);
        void SetMoveSpeed(float speed) { mMoveSpeed = speed; }
        bool IsMoved() const { return mbMoved; }
        float GetZNear() const { return mNear; }
        void SetFOV(float fov);

        void Tick(float deltaTime);

        const float3& GetPosition () const { return mPosition; }
        const float3& GetRotation () const { return mRotation; }
        const float4x4& GetWorldMatrix() const { return mWorld; }
        const float4x4& GetViewMatrix() const { return mView; }
        const float4x4& GetProjectionMatrix() const { return mProjection; }
        const float4x4& GetViewProjectionMatrix() const { return mViewProjMat; }
        float GetMoveSpeed() const { return mMoveSpeed; }
        float GetFOV() const { return mFov; }

        float3 GetLeft() const { return -mWorld[0].xyz(); }
        float3 GetRight() const { return mWorld[0].xyz(); }
        float3 GetForward() const { return mWorld[2].xyz(); }
        float3 GetBackward() const { return -mWorld[2].xyz(); }
        float3 GetUp() const { return mWorld[1].xyz(); }
        float3 GetDown() const { return -mWorld[1].xyz(); }

        void SetupCameraCB(FCameraConstants& cameraCB);
        void DrawViewFrustum(RHI::RHICommandList* pCmdList);
        void LockViewFrustum(bool value) { mbFrustumLocked = value; }
        void SetFrustumViewPosition(const float3& pos) { mFrustumViewPos = pos; }
        void UpdateFrustumPlanes(const float4x4& matrix);
        const float4* GetFrustumPlanes() const { return mFrustumPlanes; }

    private:
        void UpdateMatrix();
        void OnWindowResize(void* wndHandle, uint32_t width, uint32_t height);
        void OnCameraSettingGUI();

        void UpdateCameraPosition(float deltaTime);
        void UpdateCameraRotation(float deltaTime);

    private:
        float3 mPosition = { 0.0f, 0.0f, 0.0f };
        float3 mRotation = { 0.0f, 0.0f, 0.0f };        // in degrees

        bool mbMoved = false;

        float4x4 mWorld;
        float4x4 mView;
        float4x4 mProjection;
        float4x4 mViewProjMat;

        float mAspectRatio = 1.0f;
        float mFov = 45.0f;
        float mNear = 0.01f;
        float mFar = 1000.0f;

        float mMoveSpeed = 100.0f;
        float mRotateSpeed = 1.5f;

        float3 mPrevMoveVelocity = {};
        float2 mPrevRotateVelocity = {};
        float mMoveSmoothness = 0.01f;
        float mRotateSmoothness = 0.01f;

        bool mbFrustumLocked = false;
        float4 mFrustumPlanes[6];
        float3 mFrustumViewPos;
    };
}