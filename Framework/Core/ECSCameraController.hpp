#pragma once

#include "Renderer/CameraMath.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"

namespace Core
{
    class FCameraController
    {
    public:
        void SetAspect(float aspect) { m_State.Aspect = aspect; }
        const Renderer::FCameraMathState& GetState() const { return m_State; }
        bool IsValid() const { return m_CameraEntity != entt::null; }

        void Tick(float deltaTime, Scene::ECS::FEntitySubsystem& subsystem);

    private:
        void OnSettingsGUI();
        void UpdatePosition(float deltaTime, const float4x4& world, float3& position);
        void UpdateRotation(float deltaTime, float3& rotation);

    private:
        Renderer::FCameraMathState m_State;
        entt::entity m_CameraEntity = entt::null;
        
        float m_MoveSpeed = 100.0f;
        float m_RotateSpeed = 1.5f;
        float m_MoveSmoothness = 0.01f;
        float m_RotateSmoothness = 0.01f;
        float3 m_PrevMoveVelocity = float3(0.0f);
        float2 m_PrevRotateVelocity = float2(0.0f);

        float3 m_LastWrittenPosition = float3(0.0f);
        float4 m_LastWrittenRotation = float4(0.0f, 0.0f, 0.0f, 1.0f);
    };
}