#pragma once

#include "GlobalConstants.hlsli"

struct FInstanceData
{
    uint IndexBufferAddress;
    uint IndexStride;
    uint TriangleCount;
    uint Padding00;

    uint PositionBufferAddress;
    uint TexCoordBufferAddress;
    uint NormalBufferAddress;
    uint TangentBufferAddress;

    uint MaterialDataAddress;
    uint ObjectID;
    float Scale;
    uint Padding01;

    float3 Center;
    float Radius;

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

FInstanceData LoadInstanceData(uint instanceID)
{
    return LoadSceneConstantBuffer<FInstanceData>(SceneCB.instanceDataAddress + sizeof(FInstanceData) * instanceID);
}
#endif