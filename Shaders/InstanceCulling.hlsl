#include "Common/Common.hlsli"
#include "Common/GPUScene.hlsli"

cbuffer InstanceCullingConstants : register(b0)
{
#if FIRST_PHASE
    uint cInstanceIndexAddress;
    uint cInstanceCount;
    uint cCullingResultUAV;
    uint cInstanceListUAV2ndPhase;
    uint cInstanceListCounterUAV2ndPhase;
#else
    uint cInstanceListSRV;
    uint cInstanceListCounterSRV;
    uint cCullingResultUAV;
#endif
};

uint GetInstanceCount()
{
#if FIRST_PHASE
    uint instanceCount = cInstanceCount;
#else
    Buffer<uint> InstanceListCounter2ndPhase = ResourceDescriptorHeap[cInstanceListCounterSRV];
    uint instanceCount = InstanceListCounter2ndPhase[0];
#endif
    return instanceCount;
}

