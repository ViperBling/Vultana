#pragma once

#include "Utilities/Math.hpp"
#include "Common/GlobalConstants.hlsli"

namespace Renderer
{
    struct FCameraMathState
    {
        float3 Position = float3(0.0f);
        float3 Rotation = float3(0.0f);
        float Aspect = 1.0f;
        float Fov = 60.0f;
        float NearPlane = 0.01f;
        float FarPlane = 1000.0f;

        float4x4 World;
        float4x4 View;
        float4x4 Projection;
        float4x4 ViewProjection;
        float4x4 PrevViewProjection;
        float4x4 PrevViewProjectionInverse;
        float4 FrustumPlanes[6];
    };

    void BuildPerspective(float aspect, float fovDeg, float zNear, float4x4& outProjection);
    void ExtractFrustumPlanes(const float4x4& viewProjection, float4 outPlanes[6]);
    
    // Advances prev-frame matrices and rebuilds world/view/projection/frustum.
    void UpdateCameraMathState(FCameraMathState& state);
    void FillCameraConstants(const FCameraMathState& state, FCameraConstants& cameraCB);
}