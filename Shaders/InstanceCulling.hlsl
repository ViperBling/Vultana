#include "Common/Common.hlsli"
#include "Common/GPUScene.hlsli"
#include "Common/Stats.hlsli"

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

cbuffer BuildInstanceCullingCommandConstants : register(b0)
{
    uint cCullingCommandBufferUAV;
    uint cObjectListCounterBufferSRV;
};

cbuffer BuildMeshletListConstants : register(b0)
{
    uint cDispatchIndex;
    uint cCullingResultSRV;
    uint cOriginMeshletListAddress;
    uint cOriginMeshletCount;
    uint cMeshletListOffset;
    uint cMeshletListBufferUAV;
    uint cMeshletListBufferCounterUAV;
};

cbuffer IndirectCommandConstants : register(b0)
{
    uint cDispatchCount;
    uint cCounterBufferSRV;
    uint cCommandBufferUAV;
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

uint GetInstanceIndex(uint dispatchThreadID)
{
#if FIRST_PHASE
    ByteAddressBuffer constantBuffer = ResourceDescriptorHeap[SceneCB.SceneConstantBufferSRV];
    uint instanceIndex = constantBuffer.Load(cInstanceIndexAddress + sizeof(uint) * dispatchThreadID);
#else
    Buffer<uint> instanceList2ndPhase = ResourceDescriptorHeap[cInstanceListSRV];
    uint instanceIndex = instanceList2ndPhase[dispatchThreadID];
#endif
    return instanceIndex;
}

Texture2D<float> GetHZBTexture()
{
#if FIRST_PHASE
    Texture2D<float> hzbTexture = ResourceDescriptorHeap[SceneCB.CullingHZB1stPhaseSRV];
#else
    Texture2D<float> hzbTexture = ResourceDescriptorHeap[SceneCB.CullingHZB2ndPhaseSRV];
#endif
    return hzbTexture;
}

void CullingStats(bool visible, uint triangleCount)
{
#if FIRST_PHASE
    if (visible)
    {
        stats(STATS_1ST_PHASE_RENDERED_OBJECTS, 1);
    }
    else
    {
        stats(STATS_1ST_PHASE_CULLED_OBJECTS, 1);
        stats(STATS_1ST_PHASE_CULLED_TRIANGLE, triangleCount);
    }
#else
    if (visible)
    {
        stats(STATS_2ND_PHASE_RENDERED_OBJECTS, 1);
    }
    else
    {
        stats(STATS_2ND_PHASE_CULLED_OBJECTS, 1);
        stats(STATS_2ND_PHASE_CULLED_TRIANGLE, triangleCount);
    }
#endif
}

[numthreads(64, 1, 1)]
void InstanceCulling(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint instanceCount = GetInstanceCount();
    if (dispatchThreadID.x >= instanceCount)
    {
        return;
    }

    uint instanceIndex = GetInstanceIndex(dispatchThreadID.x);
    FInstanceData instanceData = GetInstanceData(instanceIndex);

    Texture2D<float> hzbTexture = GetHZBTexture();
    uint2 hzbSize = uint2(SceneCB.HZBWidth, SceneCB.HZBHeight);
    bool visible = OcclusionCull(hzbTexture, hzbSize, instanceData.Center, instanceData.Radius);

    RWBuffer<uint> cullingResultBuffer = ResourceDescriptorHeap[cCullingResultUAV];
    cullingResultBuffer[instanceIndex] = visible ? 1 : 0;

    CullingStats(visible, instanceData.TriangleCount);

#if FIRST_PHASE
    if (!visible)
    {
        RWBuffer<uint> instanceList2ndPhase = ResourceDescriptorHeap[cInstanceListUAV2ndPhase];
        RWBuffer<uint> instanceListCounter2ndPhase = ResourceDescriptorHeap[cInstanceListCounterUAV2ndPhase];

        uint outIndex;
        InterlockedAdd(instanceListCounter2ndPhase[0], 1, outIndex);
        instanceList2ndPhase[outIndex] = instanceIndex;
    }
#endif
}

[numthreads(1, 1, 1)]
void BuildInstanceCullingCmd(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Buffer<uint> instanceListCounter2ndPhase = ResourceDescriptorHeap[cObjectListCounterBufferSRV];
    uint instanceCount = instanceListCounter2ndPhase[0];

    RWStructuredBuffer<uint3> commandBuffer = ResourceDescriptorHeap[cCullingCommandBufferUAV];
    commandBuffer[0] = uint3((instanceCount + 63) / 64, 1, 1);
}

[numthreads(64, 1, 1)]
void BuildMeshletList(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= cOriginMeshletCount)
    {
        return;
    }

    ByteAddressBuffer constantBuffer = ResourceDescriptorHeap[SceneCB.SceneConstantBufferSRV];
    uint2 meshlet = constantBuffer.Load2(cOriginMeshletListAddress + sizeof(uint2) * dispatchThreadID.x);

    Buffer<uint> cullingResultBuffer = ResourceDescriptorHeap[cCullingResultSRV];
    bool visible = (cullingResultBuffer[meshlet.x] == 1);

    if (visible)
    {
        RWStructuredBuffer<uint2> meshletListBuffer = ResourceDescriptorHeap[cMeshletListBufferUAV];
        RWBuffer<uint> counterBuffer = ResourceDescriptorHeap[cMeshletListBufferCounterUAV];

        uint outIndex;
        InterlockedAdd(counterBuffer[cDispatchIndex], 1, outIndex);

        meshletListBuffer[cMeshletListOffset +outIndex] = meshlet;
    }
}

[numthreads(64, 1, 1)]
void BuildIndirectCmd(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint dispatchIndex = dispatchThreadID.x;
    if (dispatchIndex >= cDispatchCount)
    {
        return;
    }

    Buffer<uint> counterBuffer = ResourceDescriptorHeap[cCounterBufferSRV];
    RWStructuredBuffer<uint3> commandBuffer = ResourceDescriptorHeap[cCommandBufferUAV];

    uint meshletsCount = counterBuffer[dispatchIndex];
    commandBuffer[dispatchIndex] = uint3((meshletsCount + 31) / 32, 1, 1);
}