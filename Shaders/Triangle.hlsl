cbuffer Attributes : register(b0)
{
    uint PosAndColorIndex;
};

struct FVertex
{
    float3 positionOS;
    float3 vertexColor;
};

struct FVSOutput
{
    float4 positionCS  : SV_POSITION;
    float3 vertexColor : COLOR;
};

FVSOutput VSMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<FVertex> vertexBuffer = ResourceDescriptorHeap[PosAndColorIndex];

    FVertex vertex = vertexBuffer[vertexID];

    float3 positionOS = vertex.positionOS;
    float3 vertexColor = vertex.vertexColor;

    FVSOutput vsOut;
    vsOut.positionCS = float4(positionOS, 1.0);
    vsOut.vertexColor = vertexColor;

    return vsOut;
}

float4 PSMain(FVSOutput fsIn) : SV_TARGET
{
    return float4(fsIn.vertexColor, 1.0);
}