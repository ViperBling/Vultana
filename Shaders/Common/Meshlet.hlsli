#pragma once

struct FMeshlet
{
    float3 Center;
    float Radius;

    uint Cone;

    uint VertexCount;
    uint TriangleCount;
    uint VertexOffset;
    uint TriangleOffset;
};

struct FMeshletPayload
{
    uint InstanceIndices[32];
    uint MeshletIndices[32];
};