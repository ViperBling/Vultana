struct VSOutput
{
    float4 positionCS : SV_POSITION;
    float2 texCoord   : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput vsOut = (VSOutput)0;
    vsOut.positionCS.x = (float)(vertexID / 2) * 4.0 - 1.0;
    vsOut.positionCS.y = (float)(vertexID % 2) * 4.0 - 1.0;
    vsOut.positionCS.z = 0.0;
    vsOut.positionCS.w = 1.0;
    vsOut.texCoord.x = (float)(vertexID / 2) * 2.0;
    vsOut.texCoord.y = 1.0 - (float)(vertexID % 2) * 2.0;

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

float4 PSMain(VSOutput fsIn
    #if OUTPUT_DEPTH
    , out float outDepth : SV_Depth
    #endif
) : SV_Target
{
    Texture2D inputTexture = ResourceDescriptorHeap[cInputTexture];
    SamplerState pointSampler = SamplerDescriptorHeap[cPointSampler];

#if OUTPUT_DEPTH
    Texture2D<float> depthTexture = ResourceDescriptorHeap[cDepthTexture];
    outDepth = depthTexture.SampleLevel(pointSampler, fsIn.texCoord, 0);
#endif

    return inputTexture.SampleLevel(pointSampler, fsIn.texCoord, 0);
}

cbuffer CopytDepthTextureCB : register(b0)
{
    uint cSrcDepthTexture;
    uint cDstDepthTexture;
};

[numthreads(8, 8, 1)]
void CSCopyDepth(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float> srcDepth = ResourceDescriptorHeap[cSrcDepthTexture];
    RWTexture2D<float> dstDepth = ResourceDescriptorHeap[cDstDepthTexture];

    dstDepth[dispatchThreadID] = srcDepth[dispatchThreadID];
}