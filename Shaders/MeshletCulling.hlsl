// #include "Common/GPUScene.hlsli"
// #include "Common/Meshlet.hlsli"

// cbuffer MeshletCullingConstants : register(b0)
// {
//     uint cMeshletListBufferSRV;
//     uint cMeshletListCountSRV;

//     uint cMeshletListBufferOffset;
//     uint cDispatchIndex;
//     uint cbFirstPass;
// };

// groupshared FMeshletPayload s_Payload;

// // bool Cull(FMeshlet meshlet, uint instanceIndex, uint meshletIndex)
// // {
// //     FInstanceData instanceData = GetInstanceData(instanceIndex);
// //     float3 meshletCenter = mul(instanceData.MtxWorld, float4(meshlet.Center, 1.0f)).xyz;
// //     float radius = meshlet.Radius * instanceData.Scale;

// //     for (uint)
// // }

// [numthreads(32, 1, 1)]
// void ASMain(uint3 dispatchThreadID : SV_DispatchThreadID)
// {
    
// }