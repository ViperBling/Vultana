#include "Common/GlobalConstants.hlsli"

cbuffer FBoxVertexCB : register(b0)
{
    uint cPositionBuffer;
    uint cColorBuffer;
}

struct FVertexOutput
{
    float4 positionCS : SV_POSITION;
    float3 vertexColor : COLOR;
};

FVertexOutput VSMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[cPositionBuffer];
    StructuredBuffer<float3> colorBuffer = ResourceDescriptorHeap[cColorBuffer];

    float4 positionOS = float4(positionBuffer[vertexID], 1.0);
    float3 color = colorBuffer[vertexID];

    FVertexOutput vsOut;
    vsOut.positionCS = mul(GetCameraConstants().MtxViewProjection, positionOS);
    vsOut.vertexColor = color;

    return vsOut;
}

float4 PSMain(FVertexOutput fsIn) : SV_Target
{
    return float4(fsIn.vertexColor, 1.0);
}