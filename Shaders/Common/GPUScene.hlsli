#pragma once

#include "GlobalConstants.hlsli"

// Scene constant buffer region contract: each of the
// triple-buffered 8MB per-frame buffers reserves its final 1MB as a zeroed "null
// region". Overflowed allocations and "no material" instances point here, so every
// GPU read stays in-bounds and returns zeros. GPUScene.cpp uses
// the same constants — no local #define — and memsets the region once at creation.
static const uint MAX_SCENE_CONSTANT_BUFFER_SIZE = 1024 * 1024 * 8;
static const uint NULL_CONSTANT_REGION_SIZE = 1024 * 1024;
static const uint NULL_CONSTANT_ADDRESS = MAX_SCENE_CONSTANT_BUFFER_SIZE - NULL_CONSTANT_REGION_SIZE;
static const uint PERSISTENT_CONSTANT_REGION_SIZE = 1024 * 1024;

struct FInstanceData
{
    uint IndexBufferAddress;
    uint IndexStride;
    uint TriangleCount;
    uint _Padding00;

    uint MeshletCount;
    uint MeshletBufferAddress;
    uint MeshletVertexBufferAddress;
    uint MeshletIndexBufferAddress;

    uint PositionBufferAddress;
    uint TexCoordBufferAddress;
    uint NormalBufferAddress;
    uint TangentBufferAddress;

    uint bVertexAnimation;
    uint MaterialDataAddress;
    uint ObjectID;
    float Scale;

    float3 Center;
    float Radius;

    // uint bShowBoundingSphere;
    // uint bShowTangent;
    // uint bShowBitTangent;
    // uint bShowNormal;

    float4x4 MtxWorld;
    float4x4 MtxWorldInvTranspose;
};

#ifndef __cplusplus

template <typename T>
T LoadSceneConstantBuffer(uint bufferAddress)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[SceneCB.SceneConstantBufferSRV];
    return buffer.Load<T>(bufferAddress);
}

template <typename T>
T LoadSceneStaticBuffer(uint bufferAddress, uint elementID)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[SceneCB.SceneStaticBufferSRV];
    return buffer.Load<T>(bufferAddress + elementID * sizeof(T));
}

template <typename T>
T LoadSceneAnimationBuffer(uint bufferAddress, uint elementID)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[SceneCB.SceneAnimationBufferSRV];
    return buffer.Load<T>(bufferAddress + elementID * sizeof(T));
}

template <typename T>
void StoreSceneAnimationBuffer(uint bufferAddress, uint elementID, T value)
{
    RWByteAddressBuffer buffer = ResourceDescriptorHeap[SceneCB.SceneAnimationBufferUAV];
    buffer.Store<T>(bufferAddress + elementID * sizeof(T), value);
}

FInstanceData GetInstanceData(uint instanceID)
{
    return LoadSceneConstantBuffer<FInstanceData>(SceneCB.instanceDataAddress + sizeof(FInstanceData) * instanceID);
}
#endif