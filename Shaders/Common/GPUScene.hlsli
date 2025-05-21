#pragma once

#include "GlobalConstants.hlsli"

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
    float4x4 MtxWorldInverseTranspose;
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