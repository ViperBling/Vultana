struct FVSOutput
{
    float4 PositionCS : SV_POSITION;
    float2 TexCoord   : TEXCOORD0;
};

FVSOutput VSMain(uint vertexID : SV_VertexID)
{
    FVSOutput vsOut = (FVSOutput)0;
    vsOut.PositionCS.x = (float)(vertexID / 2) * 4.0 - 1.0;
    vsOut.PositionCS.y = (float)(vertexID % 2) * 4.0 - 1.0;
    vsOut.PositionCS.z = 0.0;
    vsOut.PositionCS.w = 1.0;
    vsOut.TexCoord.x = (float)(vertexID / 2) * 2.0;
    vsOut.TexCoord.y = 1.0 - (float)(vertexID % 2) * 2.0;

    return vsOut;
}

cbuffer CB : register(b0)
{
    uint cInputTexture;
    uint cDepthTexture;
    uint cPointSampler;
    uint padding0;
    uint padding1;
};

float4 PSMain(FVSOutput psIn
#if OUTPUT_DEPTH
    , out float outDepth : SV_DEPTH
#endif
    ) : SV_Target
{
    Texture2D inputTexture = ResourceDescriptorHeap[cInputTexture];
    SamplerState pointSampler = SamplerDescriptorHeap[cPointSampler];

#if OUTPUT_DEPTH
    Texture2D<float> depthTexture = ResourceDescriptorHeap[cDepthTexture];
    outDepth = depthTexture.SampleLevel(pointSampler, psIn.TexCoord, 0);
#endif
    return inputTexture.SampleLevel(pointSampler, psIn.TexCoord, 0);
}