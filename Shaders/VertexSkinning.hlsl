#include "Common/Common.hlsli"
#include "Common/GPUScene.hlsli"

cbuffer CB : register(b1)
{
    uint cVertexCount;
    
    uint cStaticPositionBufferAddress;
    uint cStaticNormalBufferAddress;
    uint cStaticTangentBufferAddress;

    uint cAnimationPositionBufferAddress;
    uint cAnimationNormalBufferAddress;
    uint cAnimationTangentBufferAddress;

    uint cJointIDBufferAddress;
    uint cJointWeightBufferAddress;
    uint cJointMatrixBufferAddress;
};

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchID : SV_DispatchThreadID)
{
    uint vertexID = dispatchID.x;
    if (vertexID >= cVertexCount)
    {
        return;
    }

    uint16_t4 jointID = LoadSceneStaticBuffer<uint16_t4>(cJointIDBufferAddress, vertexID);
    float4x4 jointMatrix0 = transpose(LoadSceneConstantBuffer<float4x4>(cJointMatrixBufferAddress + sizeof(float4x4) * jointID.x));
    float4x4 jointMatrix1 = transpose(LoadSceneConstantBuffer<float4x4>(cJointMatrixBufferAddress + sizeof(float4x4) * jointID.y));
    float4x4 jointMatrix2 = transpose(LoadSceneConstantBuffer<float4x4>(cJointMatrixBufferAddress + sizeof(float4x4) * jointID.z));
    float4x4 jointMatrix3 = transpose(LoadSceneConstantBuffer<float4x4>(cJointMatrixBufferAddress + sizeof(float4x4) * jointID.w));
    float4 jointWeight = LoadSceneStaticBuffer<float4>(cJointWeightBufferAddress, vertexID);

    float3 position = LoadSceneStaticBuffer<float3>(cStaticPositionBufferAddress, vertexID);

    float4 skinnedPosition = mul(jointMatrix0, float4(position, 1.0f)) * jointWeight.x + 
                             mul(jointMatrix1, float4(position, 1.0f)) * jointWeight.y + 
                             mul(jointMatrix2, float4(position, 1.0f)) * jointWeight.z + 
                             mul(jointMatrix3, float4(position, 1.0f)) * jointWeight.w;

    StoreSceneAnimationBuffer<float3>(cAnimationPositionBufferAddress, vertexID, position.xyz);

    if (cStaticNormalBufferAddress != INVALID_ADDRESS)
    {
        float3 normalOS = LoadSceneStaticBuffer<float3>(cStaticNormalBufferAddress, vertexID);

        float4 skinnedNormal = mul(jointMatrix0, float4(normalOS, 0.0f)) * jointWeight.x + 
                               mul(jointMatrix1, float4(normalOS, 0.0f)) * jointWeight.y + 
                               mul(jointMatrix2, float4(normalOS, 0.0f)) * jointWeight.z + 
                               mul(jointMatrix3, float4(normalOS, 0.0f)) * jointWeight.w;
        StoreSceneAnimationBuffer<float3>(cAnimationNormalBufferAddress, vertexID, skinnedNormal.xyz);
    }

    if (cStaticTangentBufferAddress != INVALID_ADDRESS)
    {
        float4 tangentOS = LoadSceneStaticBuffer<float4>(cStaticTangentBufferAddress, vertexID);

        float4 skinnedTangent = mul(jointMatrix0, float4(tangentOS.xyz, 0.0f)) * jointWeight.x + 
                                mul(jointMatrix1, float4(tangentOS.xyz, 0.0f)) * jointWeight.y + 
                                mul(jointMatrix2, float4(tangentOS.xyz, 0.0f)) * jointWeight.z + 
                                mul(jointMatrix3, float4(tangentOS.xyz, 0.0f)) * jointWeight.w;
        StoreSceneAnimationBuffer<float4>(cAnimationTangentBufferAddress, vertexID, float4(skinnedTangent.xyz, tangentOS.w));
    }
}