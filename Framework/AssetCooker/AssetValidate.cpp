#include "AssetCooker/AssetValidate.hpp"

#include "Utilities/Log.hpp"

namespace Assets
{
    static constexpr uint64_t kMaxBlobSize = 64ull * 1024ull * 1024ull;

    bool ValidateModel(const FModelCookedData& payload, const eastl::string& context)
    {
        // ---- meshes: MaterialIndex / IndexStride / IndexData length / blob sizes ----
        for (size_t m = 0; m < payload.Meshes.size(); ++m)
        {
            const FMeshCookedData& mesh = payload.Meshes[m];
            if (mesh.MaterialIndex < -1 || mesh.MaterialIndex >= static_cast<int32_t>(payload.Materials.size()))
            {
                VTNA_LOG_ERROR("[{}] mesh {} MaterialIndex {} out of range ({} materials)", context, m, mesh.MaterialIndex, payload.Materials.size());
                return false;
            }
            if (mesh.IndexStride != 1 && mesh.IndexStride != 2 && mesh.IndexStride != 4)
            {
                VTNA_LOG_ERROR("[{}] mesh {} IndexStride {} invalid", context, m, mesh.IndexStride);
                return false;
            }
            if (mesh.IndexData.size() != static_cast<size_t>(mesh.IndexStride) * mesh.IndexCount)
            {
                VTNA_LOG_ERROR("[{}] mesh {} IndexData {}B != stride {} * count {}", context, m, mesh.IndexData.size(), mesh.IndexStride, mesh.IndexCount);
                return false;
            }
            if (mesh.IndexData.size() > kMaxBlobSize)
            {
                VTNA_LOG_ERROR("[{}] mesh {} IndexData {}B exceeds 64MB upload limit", context, m, mesh.IndexData.size());
                return false;
            }

            bool bHasPosition = false;
            for (const FMeshCookedData::FStream& stream : mesh.Streams)
            {
                if (stream.Data.size() > kMaxBlobSize)
                {
                    VTNA_LOG_ERROR("[{}] mesh {} stream type {} exceeds 64MB upload limit", context, m, static_cast<int>(stream.Type));
                    return false;
                }
                if (stream.Stride == 0 || stream.Data.size() != static_cast<size_t>(stream.Stride) * mesh.VertexCount)
                {
                    VTNA_LOG_ERROR("[{}] mesh {} stream type {} size {}B != stride {} * vertexCount {}", context, m, static_cast<int>(stream.Type), stream.Data.size(), stream.Stride, mesh.VertexCount);
                    return false;
                }
                if (stream.Type == EVertexStreamType::Position)
                {
                    bHasPosition = true;
                    if (stream.Stride < sizeof(float3))
                    {
                        VTNA_LOG_ERROR("[{}] mesh {} position stride {} < 12B", context, m, stream.Stride);
                        return false;
                    }
                }
            }
            if (!bHasPosition || mesh.VertexCount == 0)
            {
                VTNA_LOG_ERROR("[{}] mesh {} has no position stream / zero vertices", context, m);
                return false;
            }

            if (mesh.IndexCount > 0)
            {
                const uint8_t* idx = mesh.IndexData.data();
                for (uint32_t i = 0; i < mesh.IndexCount; ++i)
                {
                    uint32_t v = 0;
                    if (mesh.IndexStride == 4)
                        v = reinterpret_cast<const uint32_t*>(idx)[i];
                    else if (mesh.IndexStride == 2)
                        v = reinterpret_cast<const uint16_t*>(idx)[i];
                    else
                        v = idx[i];
                    if (v >= mesh.VertexCount)
                    {
                        VTNA_LOG_ERROR("[{}] mesh {} index {} >= vertexCount {}", context, m, v, mesh.VertexCount);
                        return false;
                    }
                }
            }
            if (mesh.MeshletIndexData.size() > kMaxBlobSize || mesh.MeshletVertexData.size() > kMaxBlobSize)
            {
                VTNA_LOG_ERROR("[{}] mesh {} meshlet data exceeds 64MB upload limit", context, m);
                return false;
            }
            if (!mesh.bSkeletal && mesh.MeshletCount > 0 && mesh.MeshletBounds.size() != mesh.MeshletCount)
            {
                VTNA_LOG_ERROR("[{}] mesh {} MeshletBounds {} != MeshletCount {}", context, m, mesh.MeshletBounds.size(), mesh.MeshletCount);
                return false;
            }

            if (mesh.MeshletBounds.size() > kMaxBlobSize / sizeof(FMeshlet))
            {
                VTNA_LOG_ERROR("[{}] mesh {} MeshletBounds exceeds 64MB upload limit", context, m);
                return false;
            }

            for (const FMeshlet& b : mesh.MeshletBounds)
            {
                const uint64_t vEnd = static_cast<uint64_t>(b.VertexOffset) + b.VertexCount;
                const uint64_t tEnd = static_cast<uint64_t>(b.TriangleOffset) + b.TriangleCount * 3ull;
                if (vEnd > mesh.MeshletVertexData.size() / sizeof(uint32_t) ||
                    tEnd > mesh.MeshletIndexData.size() / sizeof(uint16_t))
                {
                    VTNA_LOG_ERROR("[{}] mesh {} meshlet range OOB (vEnd {} tEnd {})", context, m, vEnd, tEnd);
                    return false;
                }
            }
        }

        // ---- nodes / roots / joints: every index must land in-bounds ----
        const size_t nodeCount = payload.Nodes.size();
        const auto inRange = [nodeCount](int32_t idx)
        { return idx >= 0 && static_cast<size_t>(idx) < nodeCount; };
        for (size_t n = 0; n < nodeCount; ++n)
        {
            const FAssetNode& node = payload.Nodes[n];
            if (node.Parent != -1 && !inRange(node.Parent))
            {
                VTNA_LOG_ERROR("[{}] node {} Parent {} out of range", context, n, node.Parent);
                return false;
            }
            for (int32_t child : node.Children)
                if (!inRange(child))
                {
                    VTNA_LOG_ERROR("[{}] node {} child {} out of range", context, n, child);
                    return false;
                }
            for (int32_t prim : node.Primitives)
                if (prim < 0 || static_cast<size_t>(prim) >= payload.Meshes.size())
                {
                    VTNA_LOG_ERROR("[{}] node {} primitive {} out of range ({} meshes)", context, n, prim, payload.Meshes.size());
                    return false;
                }
        }
        for (int32_t root : payload.RootNodes)
        {
            if (!inRange(root))
            {
                VTNA_LOG_ERROR("[{}] RootNode {} out of range", context, root);
                return false;
            }
        }

        for (size_t n = 0; n < nodeCount; ++n)
        {
            const FAssetNode& node = payload.Nodes[n];
            if (node.Parent == -1)
                continue;
            if (!inRange(node.Parent))
            {
                VTNA_LOG_ERROR("[{}] node {} Parent {} out of range", context, n, node.Parent);
                return false;
            }
            // Edge consistency: the parent's Children list must include this node.
            const FAssetNode& parent = payload.Nodes[static_cast<size_t>(node.Parent)];
            bool bBackEdge = false;
            for (int32_t child : parent.Children)
                if (child == static_cast<int32_t>(n))
                {
                    bBackEdge = true;
                    break;
                }
            if (!bBackEdge)
            {
                VTNA_LOG_ERROR("[{}] node {} claims Parent {} but is not in its Children", context, n, node.Parent);
                return false;
            }
        }
        // In-degree <= 1: no node may be listed as a child by two different parents.
        eastl::vector<uint8_t> childSeen(nodeCount, 0);
        for (size_t n = 0; n < nodeCount; ++n)
        {
            const FAssetNode& node = payload.Nodes[n];
            for (int32_t child : node.Children)
            {
                if (!inRange(child))
                {
                    VTNA_LOG_ERROR("[{}] node {} child {} out of range", context, n, child);
                    return false;
                }
                if (childSeen[static_cast<size_t>(child)]++)
                {
                    VTNA_LOG_ERROR("[{}] node {} has multiple parents (child {})", context, n, child);
                    return false;
                }
            }
        }
        // Parent-chain walk: each node must reach -1 within nodeCount steps (cycle-free).
        for (size_t n = 0; n < nodeCount; ++n)
        {
            int32_t cur = static_cast<int32_t>(n);
            for (size_t step = 0; step <= nodeCount; ++step)
            {
                if (cur == -1)
                    break;
                cur = payload.Nodes[static_cast<size_t>(cur)].Parent;
            }
            if (cur != -1)
            {
                VTNA_LOG_ERROR("[{}] node graph contains a cycle (node {} does not reach a root)", context, n);
                return false;
            }
        }

        for (int32_t joint : payload.Skeleton.Joints)
            if (!inRange(joint))
            {
                VTNA_LOG_ERROR("[{}] joint {} out of range", context, joint);
                return false;
            }
        if (payload.Skeleton.InverseBindMatrices.size() != payload.Skeleton.Joints.size())
        {
            VTNA_LOG_ERROR("[{}] InverseBindMatrices {} != Joints {}", context, payload.Skeleton.InverseBindMatrices.size(), payload.Skeleton.Joints.size());
            return false;
        }

        // ---- animations: TargetNode in range, Mode known, keyframes non-empty ----
        for (const FAssetAnimation& anim : payload.Animations)
        {
            for (const FAssetAnimationChannel& channel : anim.Channels)
            {
                if (!inRange(channel.TargetNode))
                {
                    VTNA_LOG_ERROR("[{}] animation '{}' channel TargetNode {} out of range", context, anim.Name, channel.TargetNode);
                    return false;
                }
                if (channel.Mode > 2)
                {
                    VTNA_LOG_ERROR("[{}] animation '{}' channel Mode {} unknown", context, anim.Name, channel.Mode);
                    return false;
                }
                if (channel.KeyFrames.empty())
                {
                    VTNA_LOG_ERROR("[{}] animation '{}' channel TargetNode {} has no keyframes", context, anim.Name, channel.TargetNode);
                    return false;
                }
            }
        }
        return true;
    }

    bool Assets::ValidateTexture(const FTextureCookedData& payload, const eastl::string& context)
    {
        if (payload.Format == ETextureFormat::Unknown || payload.Format > ETextureFormat::BC7SRGB)
        {
            VTNA_LOG_ERROR("[ValidateTexture] {}: invalid format {}", context, (uint32_t)payload.Format);
            return false;
        }
        if (payload.Width == 0 || payload.Height == 0 || payload.Depth == 0 ||
            payload.MipLevels == 0 || payload.ArraySize == 0)
        {
            VTNA_LOG_ERROR("[ValidateTexture] {}: zero dims/levels", context);
            return false;
        }
        // v1 cooker only emits 2D single-slice; 20 mips covers any 1M^2 texture.
        if (payload.Type != ETextureType::Texture2D || payload.Depth > 1 || payload.ArraySize > 1 ||
            payload.MipLevels > 20)
        {
            VTNA_LOG_ERROR("[ValidateTexture] {}: unsupported type/shape", context);
            return false;
        }
        // MipData must exactly equal the disk-layout size. Mip dims come from the SAME canonical
        // GetTextureMipDim the cooker uses (payload.Width is the padded base) — cook and check can
        // never drift, and the dims are always 4-aligned for BC (row-pitch math exact).
        uint64_t expected = 0;
        const uint32_t blockW = GetTextureFormatBlockWidth(payload.Format);
        const uint32_t blockH = GetTextureFormatBlockHeight(payload.Format);
        for (uint32_t mip = 0; mip < payload.MipLevels; ++mip)
        {
            expected += GetTextureFormatMipSize(payload.Format, GetTextureMipDim(payload.Width, mip, blockW), GetTextureMipDim(payload.Height, mip, blockH), payload.Depth);
        }
        expected *= payload.ArraySize;
        if (payload.MipData.size() != expected)
        {
            VTNA_LOG_ERROR("[ValidateTexture] {}: mip data {} bytes, expected {}", context, payload.MipData.size(), expected);
            return false;
        }
        return true;
    }
}