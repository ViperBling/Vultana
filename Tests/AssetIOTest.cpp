#include "AssetCooker/FAssetReader.hpp"
#include "AssetCooker/FAssetWriter.hpp"

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