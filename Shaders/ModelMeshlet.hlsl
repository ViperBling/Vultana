#include "Common/Model.hlsli"
#include "Common/Meshlet.hlsli"

[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void MSMain(
    uint groupThreadID : SV_GroupThreadID,
    uint groupID : SV_GroupID,
    in payload FMeshletPayload payload,
    out indices uint3 indices[124],
    out vertices FVertexOutput vertices[64],
)
{
    uint instanceIndex = payload.InstanceIndices[groupID];
    uint meshletIndex = payload.MeshletIndices[groupID];

    FInstanceData instanceData = GetInstanceData(instanceIndex);
    if (meshletIndex >= instanceData.MeshletCount)
    {
        return;
    }

    FMeshlet meshlet = LoadSceneStaticBuffer<FMeshlet>(instanceData.MeshletBufferAddress, meshletIndex);

    SetMeshOutputCounts(meshlet.VertexCount, meshlet.TriangleCount);

    if (groupThreadID < meshlet.TriangleCount)
    {
        uint3 index = uint3(
            LoadSceneStaticBuffer<uint16_t> (instanceData.MeshletIndexBufferAddress, meshlet.TriangleOffset +groupThreadID * 3 + 0),
            LoadSceneStaticBuffer<uint16_t> (instanceData.MeshletIndexBufferAddress, meshlet.TriangleOffset +groupThreadID * 3 + 1),
            LoadSceneStaticBuffer<uint16_t> (instanceData.MeshletIndexBufferAddress, meshlet.TriangleOffset +groupThreadID * 3 + 2)
        );
        indices[groupThreadID] = index;
    }

    if (groupThreadID < meshlet.VertexCount)
    {
        uint vertexID = LoadSceneStaticBuffer<uint>(instanceData.MeshletVertexBufferAddress, meshlet.VertexOffset + groupThreadID);

        FVertexOutput vertexOut = GetVertexOutput(instanceIndex, vertexID);
        vertexOut.MeshletIndex = meshletIndex;
        vertexOut.InstanceIndex = instanceIndex;

        vertices[groupThreadID] = vertexOut;
    }
}