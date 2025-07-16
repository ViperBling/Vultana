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
        void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }
        bool IsMoved() const { return m_bMoved; }
        float GetZNear() const { return m_Near; }
        void SetFOV(float fov);

        void Tick(float deltaTime);

        const float3& GetPosition () const { return m_Position; }
        const float3& GetRotation () const { return m_Rotation; }
        const float4x4& GetWorldMatrix() const { return m_World; }
        const float4x4& GetViewMatrix() const { return m_View; }
        const float4x4& GetProjectionMatrix() const { return m_Projection; }
        const float4x4& GetViewProjectionMatrix() const { return m_ViewProjMat; }
        float GetMoveSpeed() const { return m_MoveSpeed; }
        float GetFOV() const { return m_Fov; }

        float3 GetLeft() const { return -m_World[0].xyz(); }
        float3 GetRight() const { return m_World[0].xyz(); }
        float3 GetForward() const { return m_World[2].xyz(); }
        float3 GetBackward() const { return -m_World[2].xyz(); }
        float3 GetUp() const { return m_World[1].xyz(); }
        float3 GetDown() const { return -m_World[1].xyz(); }

        void SetupCameraCB(FCameraConstants& cameraCB);
        void DrawViewFrustum(RHI::RHICommandList* pCmdList);
        void LockViewFrustum(bool value) { m_bFrustumLocked = value; }
        void SetFrustumViewPosition(const float3& pos) { m_FrustumViewPos = pos; }
        void UpdateFrustumPlanes(const float4x4& matrix);
        const float4* GetFrustumPlanes() const { return m_FrustumPlanes; }

    private:
        void UpdateMatrix();
        void OnWindowResize(void* wndHandle, uint32_t width, uint32_t height);
        void OnCameraSettingGUI();

        void UpdateCameraPosition(float deltaTime);
        void UpdateCameraRotation(float deltaTime);

    private:
        float3 m_Position = { 0.0f, 0.0f, 0.0f };
        float3 m_Rotation = { 0.0f, 0.0f, 0.0f };        // in degrees

        bool m_bMoved = false;

        float4x4 m_World;
        float4x4 m_View;
        float4x4 m_Projection;
        float4x4 m_ViewProjMat;

        float m_AspectRatio = 1.0f;
        float m_Fov = 45.0f;
        float m_Near = 0.01f;
        float m_Far = 1000.0f;

        float m_MoveSpeed = 100.0f;
        float m_RotateSpeed = 1.5f;

        float3 m_PrevMoveVelocity = {};
        float2 m_PrevRotateVelocity = {};
        float m_MoveSmoothness = 0.01f;
        float m_RotateSmoothness = 0.01f;

        bool m_bFrustumLocked = false;
        float4 m_FrustumPlanes[6];
        float3 m_FrustumViewPos;
    };
}