#include "Common/Debug.hlsli"

cbuffer RootConstants : register(b0)
{
    uint cDebugLineVertexBufferSRV;
};

struct FVSOutput
{
    float4 PositionCS : SV_Position;
    float4 VtxColor : COLOR;
};

FVSOutput VSMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<GPUDebug::FLineVertex> vertexBuffer = ResourceDescriptorHeap[cDebugLineVertexBufferSRV];
    
    FVSOutput vsOut;
    vsOut.PositionCS = mul(GetCameraConstants().MtxViewProjection, float4(vertexBuffer[vertexID].Position, 1.0f));
    vsOut.VtxColor = UnpackRGBA8Unorm(vertexBuffer[vertexID].Color);

    return vsOut;
}

float4 PSMain(FVSOutput psIn) : SV_Target
{
    return psIn.VtxColor;
}