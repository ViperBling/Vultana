struct Attributes
{
    uint positionOS;
    // uint vertexColor;
};
ConstantBuffer<Attributes> TriangleCB : register(b0);

struct VSOutput
{
    float4 positionCS  : SV_POSITION;
    float3 vertexColor : COLOR;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> posBuffer = ResourceDescriptorHeap[TriangleCB.positionOS];
    // StructuredBuffer<float3> colorBuffer = ResourceDescriptorHeap[TriangleCB.vertexColor];

    VSOutput vsOut;
    vsOut.positionCS = float4(posBuffer[vertexID], 1.0);
    vsOut.vertexColor = float3(0, 1, 1);

    return vsOut;
}

float4 PSMain(VSOutput fsIn) : SV_TARGET
{
    return float4(fsIn.vertexColor, 1.0);
}