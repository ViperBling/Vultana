#pragma once

#include "Texture.hlsli"

struct FModelMaterialConstants
{
    float3 Albedo;
    float AlphaCutout;

    float Metallic;
    float3 Specular;

    float Roughness;
    float3 Diffuse;

    float Glossiness;
    float3 Emissive;

    FMaterialTextureInfo AlbedoTexture;
    FMaterialTextureInfo NormalTexture;
    FMaterialTextureInfo MetallicRoughnessTexture;
    FMaterialTextureInfo EmissiveTexture;
    FMaterialTextureInfo AmbientOcclusionTexture;
    FMaterialTextureInfo DiffuseTexture;
    FMaterialTextureInfo SpecularGlossinessTexture;

    uint bPBRMetallicRoughness;
    uint bPBRSpecularGlossiness;
    uint bRGNormalTexture;
    uint bDoubleSided;

    uint ShadingModel;
    uint3 Padding;
};

#ifndef __cplusplus

ConstantBuffer<FModelMaterialConstants> ModelMaterialCB : register(b1);

cbuffer ModelVertexConstants : register(b2)
{
    uint cPositionBuffer;
    uint cUVBuffer;
    uint cNormalBuffer;
    uint cTangentBuffer;
};

#endif