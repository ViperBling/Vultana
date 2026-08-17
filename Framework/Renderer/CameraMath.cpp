#include "Renderer/CameraMath.hpp"

namespace Renderer
{
    void BuildPerspective(float aspect, float fovDeg, float zNear, float4x4& outProjection)
    {
        const float h = 1.0f / tan(0.5f * DegreeToRadian(fovDeg));
        const float w = h / aspect;
        outProjection = float4x4(0.0f);
        outProjection[0][0] = w;
        outProjection[1][1] = h;
        outProjection[2][2] = 0.0f;
        outProjection[2][3] = 1.0f;
        outProjection[3][2] = zNear;
    }

    void ExtractFrustumPlanes(const float4x4& m, float4 outPlanes[6])
    {
        outPlanes[0] = NormalizePlane(float4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]));
        outPlanes[1] = NormalizePlane(float4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]));
        outPlanes[2] = NormalizePlane(float4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]));
        outPlanes[3] = NormalizePlane(float4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]));
        outPlanes[4] = NormalizePlane(float4(m[0][2],           m[1][2],           m[2][2],           m[3][2]));
        outPlanes[5] = NormalizePlane(float4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]));
    }

    void UpdateCameraMathState(FCameraMathState& state)
    {
        state.PrevViewProjection = state.ViewProjection;
        state.PrevViewProjectionInverse = Inverse(state.PrevViewProjection);

        state.World = mul(translation_matrix(state.Position), rotation_matrix(RotationQuat(state.Rotation)));
        state.View = Inverse(state.World);
        BuildPerspective(state.Aspect, state.Fov, state.NearPlane, state.Projection);
        state.ViewProjection = mul(state.Projection, state.View);
        ExtractFrustumPlanes(state.ViewProjection, state.FrustumPlanes);
    }

    void FillCameraConstants(const FCameraMathState& state, FCameraConstants& cameraCB)
    {
        cameraCB.CameraPosition = state.Position;
        cameraCB.NearPlane = state.NearPlane;
        cameraCB.MtxView = state.View;
        cameraCB.MtxViewInverse = Inverse(state.View);
        cameraCB.MtxProjection = state.Projection;
        cameraCB.MtxProjectionInverse = Inverse(state.Projection);
        cameraCB.MtxViewProjection = state.ViewProjection;
        cameraCB.MtxViewProjectionInverse = Inverse(state.ViewProjection);
        cameraCB.MtxPrevViewProjection = state.PrevViewProjection;
        cameraCB.MtxPrevViewProjectionInverse = state.PrevViewProjectionInverse;
    }
} // namespace Renderer