#pragma once

#include "Common.hlsli"

namespace GPUDebug
{
    struct FLineVertex
    {
        float3 Position;
        uint Color;
    };

    void DrawLine(float3 start, float3 end, float3 color)
    {
        RWByteAddressBuffer argsBuffer = ResourceDescriptorHeap[SceneCB.DebugLineDrawCommandUAV];
        RWStructuredBuffer<FLineVertex> vertexBuffer = ResourceDescriptorHeap[SceneCB.DebugLineVertexBufferUAV];

        uint vertexCount;
        argsBuffer.InterlockedAdd(0, 2, vertexCount);
    
        FLineVertex p;
        p.Color = PackRGBA8Unorm(float4(color, 1.0));
    
        p.Position = start;
        vertexBuffer[vertexCount] = p;
    
        p.Position = end;
        vertexBuffer[vertexCount + 1] = p;
    }

    void DrawTriangle(float3 p0, float3 p1, float3 p2, float3 color)
    {
        DrawLine(p0, p1, color);
        DrawLine(p1, p2, color);
        DrawLine(p2, p0, color);
    }

    void DrawBox(float3 min, float3 max, float3 color)
    {
        float3 p0 = float3(min.x, min.y, min.z);
        float3 p1 = float3(max.x, min.y, min.z);
        float3 p2 = float3(max.x, max.y, min.z);
        float3 p3 = float3(min.x, max.y, min.z);
        float3 p4 = float3(min.x, min.y, max.z);
        float3 p5 = float3(max.x, min.y, max.z);
        float3 p6 = float3(max.x, max.y, max.z);
        float3 p7 = float3(min.x, max.y, max.z);

        DrawLine(p0, p1, color);
        DrawLine(p1, p2, color);
        DrawLine(p2, p3, color);
        DrawLine(p3, p0, color);

        DrawLine(p4, p5, color);
        DrawLine(p5, p6, color);
        DrawLine(p6, p7, color);
        DrawLine(p7, p4, color);
    
        DrawLine(p0, p4, color);
        DrawLine(p1, p5, color);
        DrawLine(p2, p6, color);
        DrawLine(p3, p7, color);
    }

    static const uint DEBUG_SPHERE_M = 10; // Latitude
    static const uint DEBUG_SPHERE_N = 20; // Longitude

    float3 SpherePosition(uint m, uint n, float3 center, float radius)
    {
        float x = sin(M_PI * m / DEBUG_SPHERE_M) * cos(2 * M_PI * n / DEBUG_SPHERE_N);
        float z = sin(M_PI * m / DEBUG_SPHERE_M) * sin(2 * M_PI * n / DEBUG_SPHERE_N);
        float y = cos(M_PI * m / DEBUG_SPHERE_M);
        return center + float3(x, y, z) * radius;
    }

    void DrawSphere(float3 center, float radius, float3 color)
    {
        for (uint m = 0; m < DEBUG_SPHERE_M; ++m)
        {
            for (uint n = 0; n < DEBUG_SPHERE_N; ++n)
            {
                float3 p0 = SpherePosition(m, n, center, radius);
                float3 p1 = SpherePosition(min(m + 1, DEBUG_SPHERE_M), n, center, radius);
                float3 p2 = SpherePosition(m, min(n + 1, DEBUG_SPHERE_N), center, radius);
            
                DrawTriangle(p0, p1, p2, color);
            }
        }
    }
}