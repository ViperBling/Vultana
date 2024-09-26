#pragma once

struct FMaterialTextureInfo
{
    uint Index;
    uint Width : 16;
    uint Height : 16;
    uint IsTransform;
    float Rotation;

    float2 Offset;
    float2 Scale;

#ifdef __cplusplus
    FMaterialTextureInfo()
    {
        Index = RHI::RHI_INVALID_RESOURCE;
        Width = Height = 0;
        IsTransform = false;
        Rotation = 0.0f;
    }
#else
    float2 TransformUV(float2 uv)
    {
        if (IsTransform)
        {
            float3x3 mtxTranslation = float3x3(1, 0, 0, 0, 1, 0, Offset.x, Offset.y, 1);
            float3x3 mtxRotation = float3x3(cos(Rotation), sin(Rotation), 0, -sin(Rotation), cos(Rotation), 0, 0, 0, 1);
            float3x3 mtxScale = float3x3(Scale.x, 0, 0, 0, Scale.y, 0, 0, 0, 1);

            return mul(mul(mul(float3(uv, 1), mtxScale), mtxRotation), mtxTranslation).xy;
        }
        return uv;
    }
#endif
};