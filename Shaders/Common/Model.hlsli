#pragma once

#include "ModelConstants.hlsli"
#include "GPUScene.hlsli"

FModelMaterialConstants GetMaterialConstants(uint instanceID)
{
    return LoadSceneConstantBuffer<FModelMaterialConstants>(GetInstanceData(instanceID).MaterialDataAddress);
}

struct FVertexAttributes
{
    float3 Position;
    float2 TexCoord;
    float3 Normal;
    float4 Tangent;
};

struct FVertexOutput
{
    float4 PositionCS : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 NormalWS : NORMAL;
    float3 TangentWS : TANGENT;
    float3 BiTangentWS : BITANGENT;
    float3 PositionWS : WORLD_POSITION;
};

FVertexAttributes GetVertexAttributes(uint instanceID, uint vertexID)
{
    FInstanceData instanceData = GetInstanceData(instanceID);

    FVertexAttributes vtx = (FVertexAttributes)0;
    vtx.TexCoord = LoadSceneStaticBuffer<float2>(instanceData.TexCoordBufferAddress, vertexID);
    vtx.Position = LoadSceneStaticBuffer<float3>(instanceData.PositionBufferAddress, vertexID);
    vtx.Normal = LoadSceneStaticBuffer<float3>(instanceData.NormalBufferAddress, vertexID);
    vtx.Tangent = LoadSceneStaticBuffer<float4>(instanceData.TangentBufferAddress, vertexID);

    return vtx;
}

FVertexOutput GetVertexOutput(uint instanceID, uint vertexID)
{
    FInstanceData instanceData = GetInstanceData(instanceID);
    FVertexAttributes vtx = GetVertexAttributes(instanceID, vertexID);

    float4 positionWS = mul(instanceData.MtxWorld, float4(vtx.Position, 1.0f));

    FVertexOutput vsOut = (FVertexOutput)0;
    vsOut.PositionCS = mul(GetCameraConstants().MtxViewProjection, positionWS);
    vsOut.PositionWS = positionWS.xyz;
    vsOut.TexCoord = vtx.TexCoord;
    vsOut.NormalWS = normalize(mul(instanceData.MtxWorldInverseTranspose, float4(vtx.Normal, 0.0f)).xyz);
    vsOut.TangentWS = normalize(mul(instanceData.MtxWorldInverseTranspose, vtx.Tangent).xyz);
    vsOut.BiTangentWS = cross(vsOut.NormalWS, vsOut.TangentWS) * vtx.Tangent.w;

    return vsOut;
}