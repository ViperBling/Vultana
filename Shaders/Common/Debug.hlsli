#pragma once

#include "Common.hlsli"

// namespace Debug
// {
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
        p.Color = 1;
    
        p.Position = start;
        vertexBuffer[vertexCount] = p;
    
        p.Position = end;
        vertexBuffer[vertexCount + 1] = p;
    }
// }