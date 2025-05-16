#include "Common/Common.hlsli"

cbuffer RootConstants : register(b0)
{
    uint cInputSRV;
    uint cOutputUAV;
    uint cHZBWidth;
    uint cHZBHeight;
};

[numthreads(8, 8, 1)]
void DepthReprojectionCS(uint3 dispatchID : SV_DispatchThreadID)
{
    
}