cbuffer ResourceCB : register(b0)
{
    uint cVertexBufferID;
    uint cVertexOffset;
    uint cTextureID;
    uint cSamplerID;
};

cbuffer ConstantBuffer : register(b1)
{
    float4x4 ProjectionMatrix;
};

struct Vertex
{
    float2 Positon;
    float2 TexCoord;
    uint Color;
};

struct VertexOutput
{
    float4 positionCS : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 vertexColor : COLOR;
};

VertexOutput VSMain(uint vertexID : SV_VertexID)
{
    StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[cVertexBufferID];
    Vertex vertex = vertexBuffer[vertexID + cVertexOffset];

    VertexOutput output = (VertexOutput)0;
    output.positionCS = mul(ProjectionMatrix, float4(vertex.Positon.xy, 0.0f, 1.0f));
    output.vertexColor = float4(1, 0, 0, 1);
    output.texCoord = vertex.TexCoord;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    Texture2D texture = ResourceDescriptorHeap[cTextureID];
    SamplerState sampler = ResourceDescriptorHeap[cSamplerID];

    float4 color = texture.Sample(sampler, input.texCoord);
    return color * input.vertexColor;
}