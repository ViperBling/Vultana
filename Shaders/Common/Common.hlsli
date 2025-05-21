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