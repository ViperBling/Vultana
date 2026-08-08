#include "AssetCooker/FAssetReader.hpp"
#include "AssetCooker/FAssetWriter.hpp"
#include "AssetCooker/FVultanaCooker.hpp"
#include "AssetCooker/FAssetReader.hpp"

#include <gtest/gtest.h>

#include <algorithm> // std::min (used by TruncateFile)
#include <cstdint>   // uint16_t
#include <cstdio>
#include <fstream>

namespace
{
    using namespace Assets;

    const char* kTestPath = "AssetIOTest_RoundtripTest.vasset";

    void WriteSyntheticModel(const char* path)
    {
        FModelCookedData src;
        FMeshCookedData mesh;
        mesh.Name = "T";
        mesh.Center = float3(1.0f, 2.0f, 3.0f);
        mesh.Radius = 1.0f;
        mesh.IndexCount = 3;
        mesh.VertexCount = 3;
        mesh.IndexStride = 2;
        uint16_t idx[] = {0, 1, 2};
        mesh.IndexData.assign((uint8_t*)idx, (uint8_t*)idx + sizeof(idx));
        FMeshCookedData::FStream pos;
        pos.Type = EVertexStreamType::Position;
        pos.Stride = 12;
        float vtx[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
        pos.Data.assign((uint8_t*)vtx, (uint8_t*)vtx + sizeof(vtx));
        mesh.Streams.push_back(eastl::move(pos));
        src.Meshes.push_back(eastl::move(mesh));

        FVAssetHeader header;
        header.SourcePath = "T/t.glb";
        header.SourceMTime = 1700000000;
        header.SourceSize = 42;
        ASSERT_TRUE(FAssetWriter::Write(path, header, src));
    }

    void TruncateFile(const char* path, size_t keepBytes)
    {
        std::ifstream is(path, std::ios::binary | std::ios::ate);
        const size_t len = (size_t)is.tellg();
        is.close();
        std::ifstream src(path, std::ios::binary);
        eastl::vector<uint8_t> buf((size_t)std::min(len, keepBytes));
        src.read((char*)buf.data(), (std::streamsize)buf.size());
        src.close();
        std::ofstream os(path, std::ios::binary | std::ios::trunc);
        os.write((const char*)buf.data(), (std::streamsize)buf.size());
    }

    const char* kCookDst = "assetcook_test.vasset";
    const char* kBoxSrc = "../Assets/Models/Box/glTF-Embedded/Box.gltf";
    const char* kMechDroneSrc = "../Assets/Models/MechDrone/scene.gltf";

} // namespace

TEST(AssetIO, Roundtrip)
{
    WriteSyntheticModel(kTestPath); // self-contained: write then read, no dependency on other tests' execution order
    FVAssetHeader header;
    FModelCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadModel(kTestPath, header, payload));
    EXPECT_EQ(header.Type, EAssetType::Model);
    EXPECT_EQ(payload.Meshes.size(), 1u);
    EXPECT_EQ((int)payload.Meshes[0].IndexCount, 3);
}

TEST(AssetIO, BadMagic)
{
    WriteSyntheticModel(kTestPath);
    {
        std::fstream f(kTestPath, std::ios::binary | std::ios::in | std::ios::out);
        uint32_t bad = 0xDEADBEEFu;
        f.write((const char*)&bad, 4);
    }
    FVAssetHeader header;
    FModelCookedData payload;
    EXPECT_FALSE(FAssetReader::ReadModel(kTestPath, header, payload));
}

TEST(AssetIO, TruncatedFile)
{
    WriteSyntheticModel(kTestPath);
    TruncateFile(kTestPath, 60); // keep only the fixed header + part of the section table
    FVAssetHeader header;
    FModelCookedData payload;
    EXPECT_FALSE(FAssetReader::ReadModel(kTestPath, header, payload));
}

TEST(AssetIO, ForgedHugeCount)
{
    // Build a hand-crafted blob whose first field is meshCount = 0xFFFFFFFF, verifying ReadBlob/ReadVector reject it
    eastl::vector<uint8_t> blob;
    {
        FBinaryWriter w;
        w.WriteU32(0xFFFFFFFFu); // forged huge meshCount
        w.WriteU32(12);          // the string length that follows
        w.Write("AAAAAAAAAAAA", 12);
        blob = w.GetData();
    }
    FBinaryReader r(blob.data(), blob.size());
    eastl::vector<uint8_t> out;
    EXPECT_FALSE(r.ReadBlob(out)); // M8: insufficient remaining bytes -> false, no resize, no crash
}

// ----------- Asset Cooker ------------------ //
TEST(AssetCook, CookBoxProducesExpectedTopology)
{
    using namespace Assets;
    ASSERT_TRUE(FVultanaCooker::CookModel(kBoxSrc, kCookDst, /*bSilent=*/true));

    FVAssetHeader header;
    FModelCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadModel(kCookDst, header, payload));
    EXPECT_EQ(payload.Meshes.size(), 1u);
    EXPECT_EQ(payload.Nodes.size(), 2u);
    EXPECT_EQ(payload.Materials.size(), 1u);
    EXPECT_EQ(payload.Meshes[0].MaterialIndex, 0);
    EXPECT_GT(payload.Meshes[0].MeshletCount, 0u); // static mesh must have meshlets
    EXPECT_FALSE(payload.Meshes[0].bSkeletal);
}

TEST(AssetCook, CookIsDeterministic)
{
    using namespace Assets;
    // Cook twice and require byte-identical output. This is the only automated guard
    // against the uninitialized-keyframe-.w bug that would otherwise make
    // ContentHash change on every cook.
    ASSERT_TRUE(FVultanaCooker::CookModel(kBoxSrc, kCookDst, /*bSilent=*/true));
    std::ifstream first(kCookDst, std::ios::binary | std::ios::ate);
    const size_t size1 = (size_t)first.tellg();
    eastl::vector<uint8_t> buf1(size1);
    first.seekg(0);
    first.read((char*)buf1.data(), size1);
    first.close();

    ASSERT_TRUE(FVultanaCooker::CookModel(kBoxSrc, kCookDst, /*bSilent=*/true));
    std::ifstream second(kCookDst, std::ios::binary | std::ios::ate);
    const size_t size2 = (size_t)second.tellg();
    eastl::vector<uint8_t> buf2(size2);
    second.seekg(0);
    second.read((char*)buf2.data(), size2);
    second.close();

    EXPECT_EQ(size1, size2);
    if (size1 == size2)
    {
        EXPECT_EQ(memcmp(buf1.data(), buf2.data(), size1), 0);
    }
}

TEST(AssetCook, CookSkeletalHasNoMeshlets)
{
    using namespace Assets;
    ASSERT_TRUE(FVultanaCooker::CookModel(kMechDroneSrc, kCookDst, /*bSilent=*/true));

    FVAssetHeader header;
    FModelCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadModel(kCookDst, header, payload));

    bool foundSkeletal = false;
    for (const FMeshCookedData& mesh : payload.Meshes)
    {
        if (mesh.bSkeletal)
        {
            foundSkeletal = true;
            EXPECT_EQ(mesh.MeshletCount, 0u); // skeletal meshes never get meshlets
            bool hasJoints = false;
            for (const FMeshCookedData::FStream& s : mesh.Streams)
            {
                if (s.Type == EVertexStreamType::Joints) hasJoints = true;
            }
            EXPECT_TRUE(hasJoints);
        }
    }
    EXPECT_TRUE(foundSkeletal);
    EXPECT_GT(payload.Skeleton.Joints.size(), 0u);
}