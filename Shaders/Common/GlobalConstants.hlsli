#pragma once

struct FCullingData
{
    float3 ViewPosition;
    float  _Padding00;

    float4 FrustumPlanes[6];
};

struct FCameraConstants
{
    float3 CameraPosition;
    float NearPlane;

    float4x4 MtxView;
    float4x4 MtxViewInverse;
    float4x4 MtxProjection;
    float4x4 MtxProjectionInverse;
    float4x4 MtxViewProjection;
    float4x4 MtxViewProjectionInverse;

    FCullingData CullingData;
};

struct FSceneConstants
{
    FCameraConstants CameraCB;

    uint SceneConstantBufferSRV;
    uint SceneStaticBufferSRV;
    uint SceneAnimationBufferSRV;
    uint SceneAnimationBufferUAV;

    float3 LightDirection;
    uint instanceDataAddress;

    float3 LightColor;
    float LightRadius;

    uint2 RenderSize;
    float2 RenderSizeInv;

    uint2 DisplaySize;
    float2 DisplaySizeInv;

    uint DebugLineDrawCommandUAV;
    uint DebugLineVertexBufferUAV;
    uint2 _Padding00;

    uint PointRepeatSampler;
    uint PointClampSampler;
    uint BilinearRepeatSampler;
    uint BilinearClampSampler;

    uint TrilinearRepeatSampler;
    uint TrilinearClampSampler;
    float FrameTime;
    uint FrameIndex;

    uint Aniso2xSampler;
    uint Aniso4xSampler;
    uint Aniso8xSampler;
    uint Aniso16xSampler;
};

#ifndef __cplusplus

ConstantBuffer<FSceneConstants> SceneCB : register(b2);

FCameraConstants GetCameraConstants()
{
    return SceneCB.CameraCB;
}

#endif