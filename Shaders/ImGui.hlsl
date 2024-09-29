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

#define IM_COL32_R_SHIFT    0
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    16
#define IM_COL32_A_SHIFT    24

float4 UnpackColor(uint color)
{
    float s = 1.0f / 255.0f;
    return float4(((color >> IM_COL32_R_SHIFT) & 0xFF) * s,
        ((color >> IM_COL32_G_SHIFT) & 0xFF) * s,
        ((color >> IM_COL32_B_SHIFT) & 0xFF) * s,
        ((color >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

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

    VertexOutput vsOut = (VertexOutput)0;
    vsOut.positionCS = mul(ProjectionMatrix, float4(vertex.Positon.xy, 0.0f, 1.0f));
    vsOut.vertexColor = UnpackColor(vertex.Color);

    vsOut.vertexColor.xyz = pow(vsOut.vertexColor.xyz, 2.2f);

    vsOut.texCoord = vertex.TexCoord;

    return vsOut;
}

float4 PSMain(VertexOutput fsIn) : SV_TARGET
{
    Texture2D uiTexture = ResourceDescriptorHeap[cTextureID];
    SamplerState uiSampler = SamplerDescriptorHeap[cSamplerID];

    float4 color = uiTexture.SampleLevel(uiSampler, fsIn.texCoord, 0) * fsIn.vertexColor;
    return color;
}