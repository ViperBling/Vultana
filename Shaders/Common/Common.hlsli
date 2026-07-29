#pragma once

#include "GlobalConstants.hlsli"

static const float M_PI = 3.141592653f;

static const uint INVALID_RESOURCE_INDEX = 0xFFFFFFFF;
static const uint INVALID_ADDRESS = 0xFFFFFFFF;

float max3(float3 v)
{
    return max(max(v.x, v.y), v.z);
}

float min3(float3 v)
{
    return min(min(v.x, v.y), v.z);
}

float square(float x)
{
    return x * x;
}

float4 UnpackRGBA8Unorm(uint packed)
{
    uint16_t4 unpacked = unpack_u8u16((uint8_t4_packed)packed);
    return unpacked / 255.0f;
}

uint PackRGBA8Unorm(float4 input)
{
    uint16_t4 unpacked = uint16_t4(input * 255.0 + 0.5);
    return (uint)pack_u8(unpacked);
}

float GetNdcDepth(float linearDepth)
{
    return GetCameraConstants().NearPlane / linearDepth; // we are using infinite far plane
}

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool ProjectSphere(float3 center, float radius, float znear, float P00, float P11, out float4 aabb)
{
    if (center.z < radius + znear)
    {
        return false;
    }

    float2 cx = -center.xz;
    float2 vx = float2(sqrt(dot(cx, cx) - radius * radius), radius);
    float2 minx = mul(cx, float2x2(float2(vx.x, vx.y), float2(-vx.y, vx.x)));
    float2 maxx = mul(cx, float2x2(float2(vx.x, -vx.y), float2(vx.y, vx.x)));

    float2 cy = -center.yz;
    float2 vy = float2(sqrt(dot(cy, cy) - radius * radius), radius);
    float2 miny = mul(cy, float2x2(float2(vy.x, vy.y), float2(-vy.y, vy.x)));
    float2 maxy = mul(cy, float2x2(float2(vy.x, -vy.y), float2(vy.y, vy.x)));

    aabb = float4(minx.x / minx.y * P00, miny.x / miny.y * P11, maxx.x / maxx.y * P00, maxy.x / maxy.y * P11);
    aabb = aabb.xwzy * float4(0.5f, -0.5f, 0.5f, -0.5f) + 0.5f; // clip space -> uv space

    return true;
}

bool OcclusionCull(Texture2D<float> hzbTexture, uint2 hzbSize, float3 center, float radius)
{
    center = mul(GetCameraConstants().MtxView, float4(center, 1.0)).xyz;
    
    bool visible = true;

    float4 aabb;
    if (ProjectSphere(center, radius, GetCameraConstants().NearPlane, GetCameraConstants().MtxProjection[0][0], GetCameraConstants().MtxProjection[1][1], aabb))
    {
        float width = (aabb.z - aabb.x) * hzbSize.x;
        float height = (aabb.w - aabb.y) * hzbSize.y;
        float2 uv = (aabb.xy + aabb.zw) * 0.5;
        float level = ceil(log2(max(width, height)));
        
    #if SUPPORTS_MIN_MAX_FILTER
        SamplerState minReductionSampler = SamplerDescriptorHeap[SceneCB.MinReductionSampler];
        float depth = hzbTexture.SampleLevel(minReductionSampler, uv, level).x;
    #else
        SamplerState pointClampSampler = SamplerDescriptorHeap[SceneCB.PointClampSampler];
        
        float2 mipSize = float2(hzbSize.x >> (uint)level, hzbSize.y >> (uint)level);
        float2 rcpMipSize = rcp(mipSize);
        float2 origin = floor(uv * mipSize - 0.5);
        float depth00 = hzbTexture.SampleLevel(pointClampSampler, (origin + float2(0.5, 0.5)) * rcpMipSize, level).x;
        float depth10 = hzbTexture.SampleLevel(pointClampSampler, (origin + float2(1.5, 0.5)) * rcpMipSize, level).x;
        float depth01 = hzbTexture.SampleLevel(pointClampSampler, (origin + float2(0.5, 1.5)) * rcpMipSize, level).x;
        float depth11 = hzbTexture.SampleLevel(pointClampSampler, (origin + float2(1.5, 1.5)) * rcpMipSize, level).x;
        float depth = min(min(depth00, depth10), min(depth01, depth11));
    #endif
        float depthSphere = GetNdcDepth(center.z - radius);

        visible = depthSphere > depth; //reversed depth
    }
    
    return visible;
}

// https://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
// Ported from RE shaders/random.hlsli:36-44 (used by meshlet debug coloring).
uint WangHash(uint x)
{
    x = (x ^ 61) ^ (x >> 16);
    x *= 9;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2d;
    x = x ^ (x >> 15);
    return x;
}