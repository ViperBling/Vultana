#include "Common/Common.hlsli"

cbuffer RootConstants : register(b0)
{
    uint cInputSRV;
    uint cOutputUAV;
    uint cHZBWidth;
    uint cHZBHeight;
};

[numthreads(8, 8, 1)]
void DepthReprojectionCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float> prevDepthTexture = ResourceDescriptorHeap[SceneCB.PrevSceneDepthSRV];
    RWTexture2D<float> reprojectedDepthTexture = ResourceDescriptorHeap[cOutputUAV];

    float2 uv = (dispatchThreadID.xy + 0.5) / float2(cHZBWidth, cHZBHeight);

    SamplerState pointClampSampler = SamplerDescriptorHeap[SceneCB.PointClampSampler];
    float4 prevNdcDepth4 = prevDepthTexture.GatherRed(pointClampSampler, uv);
    float prevNdcDepth = min(min(prevNdcDepth4.x, prevNdcDepth4.y), min(prevNdcDepth4.z, prevNdcDepth4.w));

    float4 clipPos = float4((uv * 2.0 - 1.0) * float2(1.0, -1.0), prevNdcDepth, 1.0);
    float4 worldPos = mul(GetCameraConstants().MtxViewProjectionInverse, clipPos);
    worldPos /= worldPos.w;

    float4 reprojectedPosition = mul(GetCameraConstants().MtxViewProjection, worldPos);
    reprojectedPosition /= reprojectedPosition.w;

    float reprojectedDepth = reprojectedPosition.w < 0.0f ? prevNdcDepth : reprojectedPosition.z;

    float2 reprojectedUV = reprojectedPosition.xy * 0.5 + 0.5;
    reprojectedUV.y = 1.0 - reprojectedUV.y;
    float2 reprojectedScreenPos = reprojectedUV * float2(cHZBWidth, cHZBHeight);

    reprojectedDepthTexture[reprojectedScreenPos] = saturate(reprojectedDepth);
}

[numthreads(8, 8, 1)]
void DepthDilationCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float> reprojectedDepthTexture = ResourceDescriptorHeap[cInputSRV];
    RWTexture2D<float> hzbMip0UAV = ResourceDescriptorHeap[cOutputUAV];

    float depth = reprojectedDepthTexture[dispatchThreadID.xy];

    if (depth == 0.0)
    {
        const int2 offsets[8] = {
            int2(-1, -1), int2(-1, 0), int2(-1, 1),
            int2( 0, -1),              int2( 0, 1),
            int2( 1, -1), int2( 1, 0), int2( 1, 1)
        };

        float minDepth = 1.0f;
        for (int i = 0; i < 8; ++i)
        {
            float d = reprojectedDepthTexture[dispatchThreadID.xy + offsets[i]];
            if (d > 0.0 && d < minDepth)
            {
                minDepth = d;
            }
        }

        if (minDepth != 1.0f)
        {
            depth = minDepth;
        }
    }

    hzbMip0UAV[dispatchThreadID.xy] = depth;
}

[numthreads(8, 8, 1)]
void InitHZBCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float> inputDepthTexture = ResourceDescriptorHeap[cInputSRV];
    RWTexture2D<float> outputDepthTexture = ResourceDescriptorHeap[cOutputUAV];

    float2 uv = (dispatchThreadID.xy + 0.5) / float2(cHZBWidth, cHZBHeight);

    SamplerState pointClampSampler = SamplerDescriptorHeap[SceneCB.PointClampSampler];
    float4 depth = inputDepthTexture.GatherRed(pointClampSampler, uv);
    float minDepth = min(min(depth.x, depth.y), min(depth.z, depth.w));

    outputDepthTexture[dispatchThreadID.xy] = minDepth;
}

[numthreads(8, 8, 1)]
void InitSceneHZBCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float> inputDepthTexture = ResourceDescriptorHeap[cInputSRV];
    RWTexture2D<float> outputDepthTexture = ResourceDescriptorHeap[cOutputUAV];

    float2 uv = (dispatchThreadID.xy + 0.5) / float2(cHZBWidth, cHZBHeight);

    SamplerState pointClampSampler = SamplerDescriptorHeap[SceneCB.PointClampSampler];
    float4 depth = inputDepthTexture.GatherRed(pointClampSampler, uv);
    float minDepth = min(min(depth.x, depth.y), min(depth.z, depth.w));

    outputDepthTexture[dispatchThreadID.xy] = minDepth;
}