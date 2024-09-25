cbuffer Attributes : register(b0)
{
    uint PosAndColorIndex;
};

struct Vertex
{
    float3 positionOS;
    float3 vertexColor;
};

struct VSOutput
{
    float4 positionCS  : SV_POSITION;
    float3 vertexColor : COLOR;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[PosAndColorIndex];

    Vertex vertex = vertexBuffer[vertexID];

    float3 positionOS = vertex.positionOS;
    float3 vertexColor = vertex.vertexColor;

    VSOutput vsOut;
    vsOut.positionCS = float4(positionOS, 1.0);
    vsOut.vertexColor = vertexColor;

    return vsOut;
}

float4 PSMain(VSOutput fsIn) : SV_TARGET
{
    return float4(fsIn.vertexColor, 1.0);
}