#pragma once

struct FCameraConstants
{
    float3 CameraPosition;

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

    float3 LightDirection;
    float3 LightColor;
    // float3 LightRadius;
};

#ifndef __cplusplus

ConstantBuffer<FSceneConstants> SceneCB : register(b0);

FCameraConstants GetCameraConstants()
{
    return SceneCB.CameraCB;
}

#endif