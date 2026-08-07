#include "AssetCooker/FAssetReader.hpp"
#include "AssetCooker/FAssetWriter.hpp"

// #include "Common/Meshlet.hlsli"

#include <rpmalloc/rpmalloc.h>
#include <cstdio>
#include <cstring>

namespace
{
    void PrintUsage()
    {
        printf("VultanaCooker — Vultana asset cook tool (Slice 1)\n");
        printf("Usage:\n");
        printf("  VultanaCooker roundtrip [outPath]   write a synthetic model .vasset, read it back and compare\n");
        printf("  VultanaCooker info <path>           print asset header + payload summary\n");
    }

    bool CompareModel(const Assets::FModelCookedData& modelA, const Assets::FModelCookedData& modelB)
    {
        if (modelA.Meshes.size() != modelB.Meshes.size() || modelA.Materials.size() != modelB.Materials.size() ||
            modelA.Nodes.size() != modelB.Nodes.size() || modelA.Animations.size() != modelB.Animations.size())
        {
            printf("  [FAIL] top-level count mismatch\n");
            return false;
        }
        if (modelA.RootNodes != modelB.RootNodes || modelA.Skeleton.Joints != modelB.Skeleton.Joints ||
            modelA.Skeleton.InverseBindMatrices != modelB.Skeleton.InverseBindMatrices)
        {
            printf("  [FAIL] top-level data mismatch (roots/joints/ibm)\n");
            return false;
        }
        for (size_t i = 0; i < modelA.Meshes.size(); i++)
        {
            const Assets::FMeshCookedData& m = modelA.Meshes[i];
            const Assets::FMeshCookedData& n = modelB.Meshes[i];
            if (m.Name != n.Name || m.MaterialIndex != n.MaterialIndex ||
                m.Center != n.Center || m.Radius != n.Radius ||
                m.IndexCount != n.IndexCount || m.VertexCount != n.VertexCount ||
                m.IndexStride != n.IndexStride || m.IndexData != n.IndexData ||
                m.bSkeletal != n.bSkeletal || m.bSkeletalAnim != n.bSkeletalAnim ||
                m.Streams.size() != n.Streams.size() || m.MeshletCount != n.MeshletCount ||
                m.MeshletBounds.size() != n.MeshletBounds.size() ||
                m.MeshletIndexData != n.MeshletIndexData ||
                m.MeshletVertexData != n.MeshletVertexData)
            {
                printf("  [FAIL] mesh %zu mismatch\n", i);
                return false;
            }
            for (size_t s = 0; s < m.Streams.size(); s++)
            {
                if (m.Streams[s].Type != n.Streams[s].Type ||
                    m.Streams[s].Stride != n.Streams[s].Stride ||
                    m.Streams[s].Data != n.Streams[s].Data)
                {
                    printf("  [FAIL] mesh %zu stream %zu mismatch\n", i, s);
                    return false;
                }
            }
            for (size_t k = 0; k < m.MeshletBounds.size(); k++)
            {
                if (memcmp(&m.MeshletBounds[k], &n.MeshletBounds[k], sizeof(FMeshlet)) != 0)
                {
                    printf("  [FAIL] mesh %zu meshlet bound %zu mismatch\n", i, k);
                    return false;
                }
            }
        }
        for (size_t i = 0; i < modelA.Materials.size(); i++)
        {
            const Assets::FAssetMaterial& m = modelA.Materials[i];
            const Assets::FAssetMaterial& n = modelB.Materials[i];
            if (m.Name != n.Name || m.WorkFlow != n.WorkFlow || m.ShadingModel != n.ShadingModel ||
                m.AlbedoColor != n.AlbedoColor || m.Metallic != n.Metallic || m.Roughness != n.Roughness ||
                m.DiffuseColor != n.DiffuseColor || m.SpecularColor != n.SpecularColor ||
                m.Glossiness != n.Glossiness || m.EmissiveColor != n.EmissiveColor ||
                m.AlphaCutout != n.AlphaCutout || m.bAlphaTest != n.bAlphaTest ||
                m.bAlphaBlend != n.bAlphaBlend || m.bDoubleSided != n.bDoubleSided ||
                m.Textures.size() != n.Textures.size())
            {
                printf("  [FAIL] material %zu mismatch\n", i);
                return false;
            }
            for (size_t t = 0; t < m.Textures.size(); t++)
            {
                const Assets::FAssetTextureRef& x = m.Textures[t];
                const Assets::FAssetTextureRef& y = n.Textures[t];
                if (x.RelativePath != y.RelativePath || x.Usage != y.Usage || x.bSRGB != y.bSRGB ||
                    x.bHasTransform != y.bHasTransform || x.Offset != y.Offset ||
                    x.Scale != y.Scale || x.Rotation != y.Rotation)
                {
                    printf("  [FAIL] material %zu texture %zu mismatch\n", i, t);
                    return false;
                }
            }
        }
        for (size_t i = 0; i < modelA.Nodes.size(); i++)
        {
            const Assets::FAssetNode& m = modelA.Nodes[i];
            const Assets::FAssetNode& n = modelB.Nodes[i];
            if (m.Name != n.Name || m.Parent != n.Parent || m.Children != n.Children ||
                m.Translation != n.Translation || m.Rotation != n.Rotation || m.Scale != n.Scale ||
                m.bFrontFaceCCW != n.bFrontFaceCCW || m.Primitives != n.Primitives)
            {
                printf("  [FAIL] node %zu mismatch\n", i);
                return false;
            }
        }
        for (size_t i = 0; i < modelA.Animations.size(); i++)
        {
            const Assets::FAssetAnimation& m = modelA.Animations[i];
            const Assets::FAssetAnimation& n = modelB.Animations[i];
            if (m.Name != n.Name || m.TimeDuration != n.TimeDuration ||
                m.Channels.size() != n.Channels.size())
            {
                printf("  [FAIL] animation %zu mismatch\n", i);
                return false;
            }
            for (size_t c = 0; c < m.Channels.size(); c++)
            {
                const Assets::FAssetAnimationChannel& x = m.Channels[c];
                const Assets::FAssetAnimationChannel& y = n.Channels[c];
                if (x.TargetNode != y.TargetNode || x.Mode != y.Mode ||
                    x.KeyFrames.size() != y.KeyFrames.size())
                {
                    printf("  [FAIL] animation %zu channel %zu mismatch\n", i, c);
                    return false;
                }
                for (size_t k = 0; k < x.KeyFrames.size(); k++)
                {
                    if (x.KeyFrames[k].first != y.KeyFrames[k].first ||
                        x.KeyFrames[k].second != y.KeyFrames[k].second)
                    {
                        printf("  [FAIL] animation %zu channel %zu keyframe %zu mismatch\n", i, c, k);
                        return false;
                    }
                }
            }
        }
        return true;
    }

    int RunRoundtrip(const char* outPath)
    {
        using namespace Assets;

        // ---- Build synthetic model data ----
        FModelCookedData src;

        FMeshCookedData staticMesh;
        staticMesh.Name = "SyntheticStatic";
        staticMesh.MaterialIndex = 0;
        staticMesh.Center = float3(1.0f, 2.0f, 3.0f);
        staticMesh.Radius = 5.0f;
        staticMesh.IndexCount = 3;
        staticMesh.VertexCount = 3;
        staticMesh.IndexStride = 4;
        uint32_t indices[] = {0, 1, 2};
        staticMesh.IndexData.assign((uint8_t*)indices, (uint8_t*)indices + sizeof(indices));
        FMeshCookedData::FStream posStream;
        posStream.Type = EVertexStreamType::Position;
        posStream.Stride = 12;
        float positions[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
        posStream.Data.assign((uint8_t*)positions, (uint8_t*)positions + sizeof(positions));
        staticMesh.Streams.push_back(eastl::move(posStream));
        staticMesh.MeshletCount = 1;
        FMeshlet bound = {};
        bound.Center = float3(0.5f, 0.25f, 0.0f);
        bound.Radius = 0.9f;
        bound.Cone = 0x4000007Fu; // Packed 4xint8: AxisX=127, AxisY=0, AxisZ=0, Cutoff=64 (shared FMeshlet only has uint Cone, no AxisX/Y/Z/Cutoff fields; matches MeshletCulling.hlsl's unpack_s8s16 decode)
        bound.VertexCount = 3;
        bound.TriangleCount = 1;
        bound.VertexOffset = 0;
        bound.TriangleOffset = 0;
        staticMesh.MeshletBounds.push_back(bound);
        uint16_t meshletIdx[] = {0, 1, 2};
        staticMesh.MeshletIndexData.assign((uint8_t*)meshletIdx, (uint8_t*)meshletIdx + sizeof(meshletIdx));
        uint32_t meshletVtx[] = {0, 1, 2};
        staticMesh.MeshletVertexData.assign((uint8_t*)meshletVtx, (uint8_t*)meshletVtx + sizeof(meshletVtx));
        src.Meshes.push_back(eastl::move(staticMesh));

        FMeshCookedData skeletalMesh;
        skeletalMesh.Name = "SyntheticSkeletal";
        skeletalMesh.bSkeletal = true;
        skeletalMesh.bSkeletalAnim = true;
        skeletalMesh.IndexCount = 3;
        skeletalMesh.VertexCount = 3;
        skeletalMesh.IndexStride = 2;
        uint16_t sIndices[] = {0, 1, 2};
        skeletalMesh.IndexData.assign((uint8_t*)sIndices, (uint8_t*)sIndices + sizeof(sIndices));
        FMeshCookedData::FStream jointStream;
        jointStream.Type = EVertexStreamType::Joints;
        jointStream.Stride = 8;
        uint16_t joints[] = {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
        jointStream.Data.assign((uint8_t*)joints, (uint8_t*)joints + sizeof(joints));
        skeletalMesh.Streams.push_back(eastl::move(jointStream));
        src.Meshes.push_back(eastl::move(skeletalMesh));

        FAssetMaterial mat;
        mat.Name = "SyntheticMaterial";
        mat.WorkFlow = EMaterialWorkFlow::PBRMetallicRoughness;
        mat.ShadingModel = EShadingModel::DefaultPBR;
        mat.AlbedoColor = float3(0.8f, 0.1f, 0.5f);
        mat.Metallic = 0.2f;
        mat.Roughness = 0.7f;
        FAssetTextureRef albedo;
        albedo.RelativePath = "Synthetic/albedo.BaseColor.vasset";
        albedo.Usage = ETextureUsage::BaseColor;
        albedo.bSRGB = true;
        albedo.bHasTransform = true;
        albedo.Offset = float2(0.1f, 0.2f);
        albedo.Scale = float2(2.0f, 2.0f);
        albedo.Rotation = 0.5f;
        mat.Textures.push_back(albedo);
        FAssetTextureRef normal;
        normal.RelativePath = "Synthetic/normal.Normal.vasset";
        normal.Usage = ETextureUsage::Normal;
        normal.bSRGB = false;
        mat.Textures.push_back(normal);
        src.Materials.push_back(eastl::move(mat));

        FAssetNode root;
        root.Name = "Root";
        root.Parent = -1;
        root.Translation = float3(0.0f, 1.0f, 0.0f);
        root.Scale = float3(2.0f, 2.0f, 2.0f);
        root.bFrontFaceCCW = true;   // exercise the per-node winding field in roundtrip
        root.Primitives.push_back(0);
        src.Nodes.push_back(root);
        src.RootNodes.push_back(0);

        src.Skeleton.Joints.push_back(0);
        float4x4 ibm = float4x4({identity});
        src.Skeleton.InverseBindMatrices.push_back(ibm);

        FAssetAnimation anim;
        anim.Name = "SyntheticAnim";
        FAssetAnimationChannel channel;
        channel.TargetNode = 0;
        channel.Mode = 0;
        channel.KeyFrames.push_back(eastl::make_pair(0.0f, float4(0.0f, 1.0f, 0.0f, 1.0f)));
        channel.KeyFrames.push_back(eastl::make_pair(1.0f, float4(0.0f, 2.0f, 0.0f, 1.0f)));
        anim.Channels.push_back(eastl::move(channel));
        anim.TimeDuration = 1.0f;
        src.Animations.push_back(eastl::move(anim));

        FVAssetHeader header;
        header.SourcePath = "Synthetic/synthetic.glb";
        header.SourceMTime = 1700000000;
        header.SourceSize = 123456;
        header.References.push_back("Synthetic/albedo.BaseColor.vasset");
        header.References.push_back("Synthetic/normal.Normal.vasset");

        // ---- Write ----
        if (!FAssetWriter::Write(outPath, header, src))
        {
            printf("ROUNDTRIP FAIL: write failed\n");
            return 1;
        }
        printf("  wrote %s\n", outPath);

        // ---- Read ----
        FVAssetHeader outHeader;
        FModelCookedData dst;
        if (!FAssetReader::ReadModel(outPath, outHeader, dst))
        {
            printf("ROUNDTRIP FAIL: read failed\n");
            return 1;
        }

        // ---- Compare ----
        if (outHeader.Magic != FVAssetHeader::MagicConst ||
            outHeader.Version != FVAssetHeader::CurrentVersion ||
            outHeader.Type != EAssetType::Model ||
            outHeader.SourcePath != header.SourcePath ||
            outHeader.SourceMTime != header.SourceMTime ||
            outHeader.SourceSize != header.SourceSize ||
            outHeader.References != header.References)
        {
            printf("ROUNDTRIP FAIL: header mismatch\n");
            return 1;
        }
        if (!CompareModel(src, dst))
        {
            printf("ROUNDTRIP FAIL\n");
            return 1;
        }
        printf("ROUNDTRIP OK (file kept at %s for 'info' inspection)\n", outPath);
        return 0;
    }

    int RunInfo(const char* path)
    {
        using namespace Assets;

        FVAssetHeader header;
        if (!FAssetReader::ReadHeader(path, header))
        {
            printf("INFO FAIL: cannot read header\n");
            return 1;
        }
        printf("magic:   0x%08X (expected 0x%08X)\n", header.Magic, FVAssetHeader::MagicConst);
        printf("version: %u  cooker: %u\n", header.Version, header.CookerVersion);
        printf("type:    %s\n", header.Type == EAssetType::Model ? "Model" : header.Type == EAssetType::Texture ? "Texture" : "Unknown");
        printf("source:  %s (mtime=%llu size=%llu)\n", header.SourcePath.c_str(), (unsigned long long)header.SourceMTime, (unsigned long long)header.SourceSize);
        printf("refs:    %u\n", (uint32_t)header.References.size());
        for (const eastl::string& ref : header.References)
        {
            printf("           %s\n", ref.c_str());
        }

        if (header.Type == EAssetType::Model)
        {
            FModelCookedData payload;
            FVAssetHeader header2;
            if (!FAssetReader::ReadModel(path, header2, payload))
            {
                printf("INFO FAIL: payload parse failed\n");
                return 1;
            }
            printf("meshes:   %u\n", (uint32_t)payload.Meshes.size());
            for (size_t i = 0; i < payload.Meshes.size(); i++)
            {
                const FMeshCookedData& m = payload.Meshes[i];
                printf("  [%zu] %s %s mat=%d idx=%u/%uB vtx=%u streams=%u meshlets=%u\n",
                       i, m.Name.c_str(), m.bSkeletal ? "(skeletal)" : "(static)",
                       m.MaterialIndex, m.IndexCount, m.IndexStride, m.VertexCount,
                       (uint32_t)m.Streams.size(), m.MeshletCount);
                for (size_t s = 0; s < m.Streams.size(); s++)
                {
                    printf("       stream[%zu] type=%u stride=%u bytes=%u\n",
                           s, (uint32_t)m.Streams[s].Type, m.Streams[s].Stride,
                           (uint32_t)m.Streams[s].Data.size());
                }
            }
            printf("materials: %u\n", (uint32_t)payload.Materials.size());
            for (size_t i = 0; i < payload.Materials.size(); i++)
            {
                const FAssetMaterial& mat = payload.Materials[i];
                printf("  [%zu] %s textures=%u shading=%u\n", i, mat.Name.c_str(),
                       (uint32_t)mat.Textures.size(), (uint32_t)mat.ShadingModel);
            }
            printf("nodes: %u roots: %u joints: %u anims: %u\n",
                   (uint32_t)payload.Nodes.size(), (uint32_t)payload.RootNodes.size(),
                   (uint32_t)payload.Skeleton.Joints.size(), (uint32_t)payload.Animations.size());
        }
        else if (header.Type == EAssetType::Texture)
        {
            printf("(texture payload display lands in Slice 5)\n");
        }
        printf("INFO OK\n");
        return 0;
    }
}

int main(int argc, char* argv[])
{
    rpmalloc_initialize();
    rpmalloc_thread_initialize();

    int result = 1;
    if (argc < 2)
    {
        PrintUsage();
    }
    else
    {
        eastl::string cmd = argv[1];
        if (cmd == "roundtrip")
        {
            result = RunRoundtrip(argc >= 3 ? argv[2] : "RoundtripTest.vasset");
        }
        else if (cmd == "info")
        {
            if (argc < 3)
            {
                PrintUsage();
            }
            else
            {
                result = RunInfo(argv[2]);
            }
        }
        else
        {
            PrintUsage();
        }
    }

    rpmalloc_thread_finalize(1);
    rpmalloc_finalize();
    return result;
}