#pragma once

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
};

struct FSceneConstants
{
    FCameraConstants CameraCB;

    uint SceneConstantBufferSRV;
    uint SceneStaticBufferSRV;
    uint instanceDataAddress;
    uint Padding00;

    float3 LightDirection;
    uint Padding01;

    float3 LightColor;
    float LightRadius;
};

#ifndef __cplusplus

ConstantBuffer<FSceneConstants> SceneCB : register(b2);

FCameraConstants GetCameraConstants()
{
    return SceneCB.CameraCB;
}

#endif