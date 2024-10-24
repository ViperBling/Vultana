#include "Common/Common.hlsli"
#include "Common/GPUScene.hlsli"

cbuffer CB : register(b0)
{
    uint cOutUAV;
    uint cConstantSRV;
    uint cStaticSRV;
    uint cInstanceAddress;
    uint cAnimationSRV;
};

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchID : SV_DispatchThreadID)
{
    RWTexture2D<float4> outTexture = ResourceDescriptorHeap[cOutUAV];
    ByteAddressBuffer buffer = ResourceDescriptorHeap[cAnimationSRV];
    float3 data = buffer.Load<float3>(0 + 12 * 80);

    float3 testVal = data;
    
    // outTexture[dispatchID.xy] = float4(float(dispatchID.x / 1280.0f), float(dispatchID.y / 720.0f), 0.0f, 1.0f);
    outTexture[dispatchID.xy] = float4(testVal, 1.0f);
}