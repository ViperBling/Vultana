struct Attributes
{
    float3 positionOS  : POSITION;
    float4 vertexColor : COLOR;
};

struct Varyings
{
    float4 positionCS  : SV_POSITION;
    float4 vertexColor : COLOR;
};

Varyings VSMain(Attributes vsIn)
{
    Varyings vsOut;
    vsOut.positionCS = vsIn.positionOS;

    vsOut.vertexColor = vsIn.vertexColor;
    return vsOut;
}

float4 PSMain(Varyings fsIn) : SV_TARGET
{
    return fsIn.vertexColor;
}