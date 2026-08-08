#include "AssetCooker/FMeshCooker.hpp"
#include "Utilities/Math.hpp"

#include <meshoptimizer.h>
#include <EASTL/vector.h>
#include <EASTL/algorithm.h>

#include <cassert>
#include <cstring>

namespace Assets
{
    // Widen 8-bit indices to 16-bit (RHI exposes only R16UI/R32UI index formats). Called
    // after meshlet construction for statics, and unconditionally for skeletal.
    static void WidenIndicesIfNeeded(FMeshCookedData& out)
    {
        if (out.IndexStride != 1)
        {
            return;
        }
        eastl::vector<uint8_t> widened(out.IndexCount * 2);
        uint16_t* dst = reinterpret_cast<uint16_t*>(widened.data());
        for (uint32_t i = 0; i < out.IndexCount; i++)
        {
            dst[i] = out.IndexData[i];
        }
        out.IndexData = eastl::move(widened);
        out.IndexStride = 2;
    }

    // Full static bake for one index type: meshopt remap (index + vertex streams), meshlet
    // build, bounds, and the final widen-to-16. Templated over the index type because
    // meshopt overloads generateVertexRemapMulti / remapIndexBuffer / buildMeshlets per
    // index type (uint8/16/32); CookStatic dispatches by IndexStride.
    template <typename TIndex>
    static bool RemapAndBuildMeshlets(const FImportPrimitive& in, FMeshCookedData& out)
    {
        const size_t indexCount = in.IndexCount;
        const size_t vertexCount = in.VertexCount;

        // Joints/Weights never participate in the static remap (matches the old static path,
        // where they fell through to `default: break`); including them would change the
        // dedup equivalence and therefore the meshlet layout.
        eastl::vector<meshopt_Stream> streams;
        streams.reserve(in.Streams.size());
        for (const FImportVertexStream& s : in.Streams)
        {
            if (s.Type == EVertexStreamType::Joints || s.Type == EVertexStreamType::Weights)
            {
                continue;
            }
            meshopt_Stream ms;
            ms.data = const_cast<uint8_t*>(s.Data.data());
            ms.size = s.Stride;
            ms.stride = s.Stride;
            streams.push_back(ms);
        }
        if (streams.empty())
        {
            return false;
        }

        // remap table needs vertexCount entries (meshopt contract), not indexCount; take the
        // max so indexCount < vertexCount (shared POSITION accessor) cannot overflow.
        eastl::vector<unsigned int> remap(eastl::max<size_t>(indexCount, vertexCount));
        const size_t remappedVertexCount = meshopt_generateVertexRemapMulti(remap.data(), reinterpret_cast<const TIndex*>(in.IndexData.data()), indexCount, vertexCount, streams.data(), streams.size());
        if (remappedVertexCount == 0)
        {
            return false;
        }

        // Remap index buffer; the widen-to-16 happens after meshlet construction (meshopt already consumed the original stride).
        eastl::vector<uint8_t> remappedIndices(indexCount * sizeof(TIndex));
        meshopt_remapIndexBuffer(reinterpret_cast<TIndex*>(remappedIndices.data()), reinterpret_cast<const TIndex*>(in.IndexData.data()), indexCount, remap.data());
        out.IndexData = eastl::move(remappedIndices);
        out.VertexCount = static_cast<uint32_t>(remappedVertexCount);

        // Remap each vertex stream (skip Joints/Weights, same filter as above)
        size_t posStreamIndex = SIZE_MAX;
        size_t posStride = 0;
        out.Streams.reserve(in.Streams.size());
        for (size_t i = 0; i < in.Streams.size(); i++)
        {
            const FImportVertexStream& src = in.Streams[i];
            if (src.Type == EVertexStreamType::Joints || src.Type == EVertexStreamType::Weights)
            {
                continue;
            }
            FMeshCookedData::FStream dst;
            dst.Type = src.Type;
            dst.Stride = src.Stride;
            dst.Data.resize(src.Stride * remappedVertexCount);
            meshopt_remapVertexBuffer(dst.Data.data(), src.Data.data(), vertexCount, src.Stride, remap.data());
            if (src.Type == EVertexStreamType::Position)
            {
                posStreamIndex = out.Streams.size();
                posStride = src.Stride;
            }
            out.Streams.push_back(eastl::move(dst));
        }
        
        if (posStreamIndex == SIZE_MAX || posStride < sizeof(float3))
        {
            return false;
        }
        const void* posVertices = out.Streams[posStreamIndex].Data.data();

        // ---- Meshlet build ----
        const size_t maxVertices = 64;
        const size_t maxTriangles = 124;
        const float coneWeight = 0.5f;
        const size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, maxVertices, maxTriangles);

        eastl::vector<meshopt_Meshlet> meshlets(maxMeshlets);
        eastl::vector<unsigned int> meshletVertices(maxMeshlets * maxVertices);
        eastl::vector<unsigned char> meshletTriangles(maxMeshlets * maxTriangles * 3);

        const size_t meshletCount = meshopt_buildMeshlets(
            meshlets.data(), meshletVertices.data(), meshletTriangles.data(),
            reinterpret_cast<const TIndex*>(out.IndexData.data()), indexCount,
            reinterpret_cast<const float*>(posVertices), remappedVertexCount, posStride,
            maxVertices, maxTriangles, coneWeight);

        // Trim to actual counts
        if (meshletCount == 0)
        {
            return false; // No meshlets (degenerate input); matches old loader's implicit assumption
        }
        const meshopt_Meshlet& lastMeshlet = meshlets[meshletCount - 1];
        meshletVertices.resize(lastMeshlet.vertex_offset + lastMeshlet.vertex_count);
        meshletTriangles.resize(lastMeshlet.triangle_offset + ((lastMeshlet.triangle_count * 3 + 3) & ~3));
        meshlets.resize(meshletCount);

        // Triangle per meshlet -> uint16 (meshopt emits uint8 per triangle * 3)
        eastl::vector<unsigned short> meshletTriangles16;
        meshletTriangles16.reserve(meshletTriangles.size());
        for (size_t i = 0; i < meshletTriangles.size(); i++)
        {
            meshletTriangles16.push_back(meshletTriangles[i]);
        }

        eastl::vector<FMeshlet> meshletBounds(meshletCount);
        for (size_t i = 0; i < meshletCount; i++)
        {
            const meshopt_Meshlet& meshlet = meshlets[i];
            meshopt_Bounds mb = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, reinterpret_cast<const float*>(posVertices), remappedVertexCount, posStride);

            FMeshlet bound;
            bound.Center = float3(mb.center);
            bound.Radius = mb.radius;
            bound.AxisX = mb.cone_axis_s8[0];
            bound.AxisY = mb.cone_axis_s8[1];
            bound.AxisZ = mb.cone_axis_s8[2];
            bound.Cutoff = mb.cone_cutoff_s8;
            bound.VertexCount = meshlet.vertex_count;
            bound.TriangleCount = meshlet.triangle_count;
            bound.VertexOffset = meshlet.vertex_offset;
            bound.TriangleOffset = meshlet.triangle_offset;
            meshletBounds[i] = bound;
        }

        out.MeshletCount = static_cast<uint32_t>(meshletCount);
        out.MeshletBounds = eastl::move(meshletBounds);
        out.MeshletIndexData.resize(meshletTriangles16.size() * sizeof(unsigned short));
        memcpy(out.MeshletIndexData.data(), meshletTriangles16.data(), out.MeshletIndexData.size());
        out.MeshletVertexData.resize(meshletVertices.size() * sizeof(unsigned int));
        memcpy(out.MeshletVertexData.data(), meshletVertices.data(), out.MeshletVertexData.size());

        WidenIndicesIfNeeded(out);
        return true;
    }

    bool FMeshCooker::Cook(const FImportPrimitive& in, FMeshCookedData& out)
    {
        out.Name = in.Name;
        out.MaterialIndex = in.MaterialIndex;
        out.Center = in.Center;
        out.Radius = in.Radius;
        out.bSkeletal = in.bSkeletal;
        out.bSkeletalAnim = in.bSkeletalAnim;

        if (in.IndexData.empty())
        {
            return false;
        }

        // ---- Index data ----
        out.IndexStride = in.IndexStride;
        out.IndexCount = in.IndexCount;

        if (in.bSkeletal)
        {
            return CookSkeletal(in, out);
        }
        return CookStatic(in, out);
    }

    bool FMeshCooker::CookSkeletal(const FImportPrimitive& in, FMeshCookedData& out)
    {
        // Skeletal: no meshopt remap, no meshlet (matches old LoadSkeletalMeshData).
        out.VertexCount = in.VertexCount;
        out.IndexData = in.IndexData;
        for (const FImportVertexStream& s : in.Streams)
        {
            out.Streams.push_back(s);
        }
        // Widen 8-bit indices to 16-bit: the RHI only exposes R16UI/R32UI index formats.
        WidenIndicesIfNeeded(out);
        out.MeshletCount = 0;
        return true;
    }

    bool FMeshCooker::CookStatic(const FImportPrimitive& in, FMeshCookedData& out)
    {
        if (in.VertexCount == 0)
        {
            return false;
        }
        // Index-type dispatch: meshopt overloads remap/remapIndex/buildMeshlets per index type.
        switch (out.IndexStride)
        {
        case 4: return RemapAndBuildMeshlets<uint32_t>(in, out);
        case 2: return RemapAndBuildMeshlets<uint16_t>(in, out);
        case 1: return RemapAndBuildMeshlets<uint8_t>(in, out);
        default:
            assert(false);
            return false;
        }
    }
} // namespace Assets
