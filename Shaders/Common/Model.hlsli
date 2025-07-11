#pragma once

#include "ModelConstants.hlsli"
#include "GPUScene.hlsli"
#include "Debug.hlsli"

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

    // nointerpolation uint MeshletIndex : COLOR0;
    // nointerpolation uint InstanceIndex : COLOR1;
};

FVertexAttributes GetVertexAttributes(uint instanceID, uint vertexID)
{
    FInstanceData instanceData = GetInstanceData(instanceID);

    FVertexAttributes vtx = (FVertexAttributes)0;
    vtx.TexCoord = LoadSceneStaticBuffer<float2>(instanceData.TexCoordBufferAddress, vertexID);

    if (instanceData.bVertexAnimation)
    {
        vtx.Position = LoadSceneAnimationBuffer<float3>(instanceData.PositionBufferAddress, vertexID);
        vtx.Normal = LoadSceneAnimationBuffer<float3>(instanceData.NormalBufferAddress, vertexID);
        vtx.Tangent = LoadSceneAnimationBuffer<float4>(instanceData.TangentBufferAddress, vertexID);
    }
    else
    {
        vtx.Position = LoadSceneStaticBuffer<float3>(instanceData.PositionBufferAddress, vertexID);
        vtx.Normal = LoadSceneStaticBuffer<float3>(instanceData.NormalBufferAddress, vertexID);
        vtx.Tangent = LoadSceneStaticBuffer<float4>(instanceData.TangentBufferAddress, vertexID);
    }
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
    vsOut.TangentWS = normalize(mul(instanceData.MtxWorldInverseTranspose, float4(vtx.Tangent.xyz, 0.0f)).xyz);
    vsOut.BiTangentWS = cross(vsOut.NormalWS, vsOut.TangentWS) * vtx.Tangent.w;

    // GPUDebug::DrawLine(vsOut.PositionWS.xyz, vsOut.PositionWS.xyz + vsOut.NormalWS * 1.0f, float3(0.0f, 0.0f, 1.0f));
    // if (vertexID == 0)
    // {
    //     GPUDebug::DrawSphere(instanceData.Center, instanceData.Radius, float3(1.0f, 0.0f, 0.0f));
    // }

    return vsOut;
}

struct FPBRMetallicRoughness
{
    float3 Albedo;
    float Alpha;
    float Metallic;
    float Roughness;
    float AmbientOcclusion;
};

struct FPBRSpecularGlossiness
{
    float3 Diffuse;
    float Alpha;
    float3 Specular;
    float Glossiness;
};

Texture2D GetMaterialTexture2D(uint heapIndex)
{
    return ResourceDescriptorHeap[heapIndex];
}

SamplerState GetMaterialSampler()
{
    return SamplerDescriptorHeap[SceneCB.Aniso16xSampler];
}

float4 SampleMaterialTexture(FMaterialTextureInfo textureInfo, float2 texCoord, float mipLOD = 0)
{
    Texture2D texture = GetMaterialTexture2D(textureInfo.Index);
    SamplerState anisoSampler = GetMaterialSampler();
    texCoord = textureInfo.TransformUV(texCoord);

    return texture.SampleLevel(anisoSampler, texCoord, mipLOD);
}

void AlphaTest(uint instanceID, float2 uv)
{
    float alpha = 1.0;
#if ALBEDO_TEXTURE
    alpha = SampleMaterialTexture(GetMaterialConstants(instanceID).AlbedoTexture, uv, 0).a;
#elif DIFFUSE_TEXTURE
    alpha = SampleMaterialTexture(GetMaterialConstants(instanceID).DiffuseTexture, uv, 0).a;
#endif
    clip(alpha - GetMaterialConstants(instanceID).AlphaCutout);
}