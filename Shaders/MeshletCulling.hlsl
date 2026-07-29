#include "Common/Common.hlsli"
#include "Common/GPUScene.hlsli"
#include "Common/Meshlet.hlsli"
#include "Common/Stats.hlsli"

cbuffer MeshletCullingConstants : register(b0)
{
    uint cMeshletListBufferSRV;
    uint cMeshletListCountSRV;

    uint cMeshletListBufferOffset;
    uint cDispatchIndex;
    uint cbFirstPass;
};

groupshared FMeshletPayload s_Payload;

bool Cull(FMeshlet meshlet, uint instanceIndex, uint meshletIndex)
{
    FInstanceData instanceData = GetInstanceData(instanceIndex);
    float3 meshletCenter = mul(instanceData.MtxWorld, float4(meshlet.Center, 1.0f)).xyz;
    float radius = meshlet.Radius * instanceData.Scale;

    // 1. Frustum culling
    for (uint i = 0; i < 6; ++i)
    {
        if (dot(meshletCenter, GetCameraConstants().CullingData.FrustumPlanes[i].xyz) + GetCameraConstants().CullingData.FrustumPlanes[i].w + radius < 0)
        {
            stats(cbFirstPass ? STATS_1ST_PHASE_FRUSTUM_CULLED_MESHLET : STATS_2ND_PHASE_FRUSTUM_CULLED_MESHLET, 1);
            return false;
        }
    }

#if !DOUBLE_SIDED
    // 2. Backface culling
    int16_t4 cone = unpack_s8s16((int8_t4_packed) meshlet.Cone);
    float3 axis = cone.xyz / 127.0;
    float cutoff = cone.w / 127.0;

    axis = normalize(mul(instanceData.MtxWorld, float4(axis, 0.0)).xyz);
    float3 view = meshletCenter - GetCameraConstants().CullingData.ViewPosition;

    if (dot(view, -axis) >= cutoff * length(view) + radius)
    {
        stats(cbFirstPass ? STATS_1ST_PHASE_BACKFACE_CULLED_MESHLET : STATS_2ND_PHASE_BACKFACE_CULLED_MESHLET, 1);
        return false;
    }
#endif

    // 3. Occlusion culling
    Texture2D<float> hzbTexture = ResourceDescriptorHeap[cbFirstPass ? SceneCB.CullingHZB1stPhaseSRV : SceneCB.CullingHZB2ndPhaseSRV];
    uint2 hzbSize = uint2(SceneCB.HZBWidth, SceneCB.HZBHeight);
    
    if (!OcclusionCull(hzbTexture, hzbSize, meshletCenter, radius))
    {
        if (cbFirstPass)
        {
            RWBuffer<uint> counterBuffer = ResourceDescriptorHeap[SceneCB.SecondPhaseMeshletsCounterUAV];
            RWStructuredBuffer<uint2> occlusionCulledMeshletsBuffer = ResourceDescriptorHeap[SceneCB.SecondPhaseMeshletsListUAV];
        
            uint outIndex;
            InterlockedAdd(counterBuffer[cDispatchIndex], 1, outIndex);
        
            occlusionCulledMeshletsBuffer[cMeshletListBufferOffset +outIndex] = uint2(instanceIndex, meshletIndex);
        
            stats(STATS_1ST_PHASE_OCCLUSION_CULLED_MESHLET, 1);
        }
        else
        {
            stats(STATS_2ND_PHASE_OCCLUSION_CULLED_MESHLET, 1);
        }
    
        return false;
    }
    
    stats(cbFirstPass ? STATS_1ST_PHASE_RENDERED_MESHLET : STATS_2ND_PHASE_RENDERED_MESHLET, 1);
    return true;
}

[numthreads(32, 1, 1)]
void ASMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Buffer<uint> counterBuffer = ResourceDescriptorHeap[cMeshletListCountSRV];
    uint totalMeshletCount = counterBuffer[cDispatchIndex];

    bool visible = false;
    if (dispatchThreadID.x < totalMeshletCount)
    {
        StructuredBuffer<uint2> meshletsListBuffer = ResourceDescriptorHeap[cMeshletListBufferSRV];
        uint2 dataPerMeshlet = meshletsListBuffer[cMeshletListBufferOffset + dispatchThreadID.x];
        uint instanceIndex = dataPerMeshlet.x;
        uint meshletIndex = dataPerMeshlet.y;

        FMeshlet meshlet = LoadSceneStaticBuffer<FMeshlet>(GetInstanceData(instanceIndex).MeshletBufferAddress, meshletIndex);

        visible = Cull(meshlet, instanceIndex, meshletIndex);

        if (cbFirstPass)
        {
            stats(visible ? STATS_1ST_PHASE_RENDERED_TRIANGLE : STATS_1ST_PHASE_CULLED_TRIANGLE, meshlet.TriangleCount);
        }
        else
        {
            stats(visible ? STATS_2ND_PHASE_RENDERED_TRIANGLE : STATS_2ND_PHASE_CULLED_TRIANGLE, meshlet.TriangleCount);
        }

        if (visible)
        {
            uint index = WavePrefixCountBits(visible);
            s_Payload.InstanceIndices[index] = instanceIndex;
            s_Payload.MeshletIndices[index] = meshletIndex;
        }
    }

    uint visibleMeshletCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleMeshletCount, 1, 1, s_Payload);
}