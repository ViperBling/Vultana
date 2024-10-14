#include "Common/GlobalConstants.hlsli"
#include "Common/ModelConstants.hlsli"

struct FVertexOutput
{
    float4 positionCS : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normalWS : NORMAL;
    float3 tangentWS : TANGENT;
    float3 positionWS : TEXCOORD1;
};

FVertexOutput VSMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[ModelCB.PositionBuffer];
    StructuredBuffer<float2> texCoordBuffer = ResourceDescriptorHeap[ModelCB.TexCoordBuffer];
    // StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[ModelCB.NormalBuffer];
    // StructuredBuffer<float3> tangentBuffer = ResourceDescriptorHeap[ModelCB.TangentBuffer];

    float4 positionOS = float4(positionBuffer[vertexID], 1.0f);
    FVertexOutput vsOut = (FVertexOutput)0;
    vsOut.positionCS = mul(GetCameraConstants().MtxViewProjection, positionOS);
    vsOut.texCoord = texCoordBuffer[vertexID];
    // vsOut.normalWS = mul(ModelCB.MtxWorldInverse, float4(normalBuffer[vertexID], 0.0f)).xyz;
    // vsOut.tangentWS = mul(ModelCB.MtxWorldInverse, float4(tangentBuffer[vertexID], 0.0f)).xyz;
    vsOut.positionWS = positionOS.xyz;

    return vsOut;
}

float4 PSMain(FVertexOutput psIn) : SV_Target
{
    return float4(psIn.texCoord.xy, 0, 1.0f);
}