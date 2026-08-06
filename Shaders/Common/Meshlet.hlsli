#pragma once

#ifdef __cplusplus
#include <cstdint>
// float3/uint 由 Utilities/Math.hpp 提供,include 本头前需先 include Math.hpp
#endif

// CPU/GPU 共享的 meshlet 包围球 + 锥体剔除结构(36 字节)。
// 布局与 meshopt_computeMeshletBounds 输出一致;改动必须同步 C++ 侧断言。
struct FMeshlet
{
    float3 Center;
    float Radius;

#ifdef __cplusplus
    union
    {
        struct
        {
            int8_t AxisX;
            int8_t AxisY;
            int8_t AxisZ;
            int8_t Cutoff;
        };
        uint32_t Cone;
    };
#else
    uint Cone;
#endif

    uint VertexCount;
    uint TriangleCount;

    uint VertexOffset;
    uint TriangleOffset;
};

#ifdef __cplusplus
static_assert(sizeof(FMeshlet) == 36, "FMeshlet layout must stay byte-identical to the GPU side");
#endif

struct FMeshletPayload
{
    uint InstanceIndices[32];
    uint MeshletIndices[32];
};