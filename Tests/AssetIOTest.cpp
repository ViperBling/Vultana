#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetCooker/AssetReader.hpp"
#include "AssetCooker/AssetWriter.hpp"
#include "AssetCooker/AssetValidate.hpp"
#include "AssetCooker/VultanaCooker.hpp"
#include "AssetCooker/AssimpImporter.hpp"

#include "RHI/RHI.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <sys/utime.h>

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
    const char* kStaleCopy = "assetcook_stale_src.gltf";

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

TEST(AssetIO, RoundtripPreservesMaterialWorkFlow)
{
    // P0 regression: ReadModelPayload's missing { return false; } braces made the
    // material WorkFlow assignment the body of the bounds check — on a VALID file the
    // condition is false, so WorkFlow was never read and every material silently
    // degraded to PBRMetallicRoughness (the default zero-init). A SpecularGlossiness
    // source must come back as SpecularGlossiness.
    FModelCookedData src;
    FMeshCookedData mesh;
    mesh.Name = "T";
    mesh.MaterialIndex = 0;
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

    FAssetMaterial mat;
    mat.Name = "SG";
    mat.WorkFlow = EMaterialWorkFlow::PBRSpecularGlossiness;
    src.Materials.push_back(eastl::move(mat));

    FVAssetHeader header;
    header.SourcePath = "T/t.glb";
    header.SourceMTime = 1700000000;
    header.SourceSize = 42;
    ASSERT_TRUE(FAssetWriter::Write(kTestPath, header, src));

    FVAssetHeader outHeader;
    FModelCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadModel(kTestPath, outHeader, payload));
    ASSERT_EQ(payload.Materials.size(), 1u);
    EXPECT_EQ((int)payload.Materials[0].WorkFlow, (int)EMaterialWorkFlow::PBRSpecularGlossiness);
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
    for (const char* src : { kBoxSrc, kMechDroneSrc })
    {
        ASSERT_TRUE(FVultanaCooker::CookModel(src, kCookDst, /*bSilent=*/true));
        std::ifstream first(kCookDst, std::ios::binary | std::ios::ate);
        const size_t size1 = static_cast<size_t>(first.tellg());
        eastl::vector<uint8_t> buf1(size1);
        first.seekg(0);
        first.read(reinterpret_cast<char*>(buf1.data()), size1);
        first.close();

        ASSERT_TRUE(FVultanaCooker::CookModel(src, kCookDst, /*bSilent=*/true));
        std::ifstream second(kCookDst, std::ios::binary | std::ios::ate);
        const size_t size2 = static_cast<size_t>(second.tellg());
        eastl::vector<uint8_t> buf2(size2);
        second.seekg(0);
        second.read(reinterpret_cast<char*>(buf2.data()), size2);
        second.close();

        EXPECT_EQ(size1, size2);
        if (size1 == size2)
        {
            EXPECT_EQ(memcmp(buf1.data(), buf2.data(), size1), 0);
        }
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

TEST(AssetCook, CookAnimationHasDuration)
{
    // P1 regression: the per-channel TimeDuration update was left commented out, so every
    // cooked animation carried duration 0 and FAnimation::Update never wrapped the play
    // head — animation froze at the first frame after one pass (MechDrone/BusterDrone).
    using namespace Assets;
    ASSERT_TRUE(FVultanaCooker::CookModel(kMechDroneSrc, kCookDst, /*bSilent=*/true));

    FVAssetHeader header;
    FModelCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadModel(kCookDst, header, payload));
    ASSERT_FALSE(payload.Animations.empty());
    EXPECT_GT(payload.Animations[0].TimeDuration, 0.0f);
}

TEST(AssetCook, IsUpToDateDetectsStaleSource)
{
    using namespace Assets;
    {
        std::ifstream src(kBoxSrc, std::ios::binary);
        std::ofstream copy(kStaleCopy, std::ios::binary);
        copy << src.rdbuf();
    }
    ASSERT_TRUE(FVultanaCooker::CookModel(kStaleCopy, kCookDst, /*bSilent=*/true));
    EXPECT_TRUE(FVultanaCooker::IsUpToDate(kStaleCopy, kCookDst));

    struct __utimbuf64 times = {};
    _utime64(kStaleCopy, &times); // reads current mtime into times
    times.modtime += 100;         // now definitely != what the cook recorded
    times.actime = times.modtime;
    _utime64(kStaleCopy, &times);

    EXPECT_FALSE(FVultanaCooker::IsUpToDate(kStaleCopy, kCookDst));
}

// ---- AssetValidate ----
namespace
{
    void AddPositionStream(Assets::FMeshCookedData& mesh, uint32_t vertexCount)
    {
        Assets::FMeshCookedData::FStream pos;
        pos.Type = Assets::EVertexStreamType::Position;
        pos.Stride = sizeof(float3);
        pos.Data.resize(pos.Stride * vertexCount);
        mesh.VertexCount = vertexCount;
        mesh.Streams.push_back(eastl::move(pos));
    }
}

TEST(AssetValidate, RejectsOutOfRangeMaterialIndex)
{
    using namespace Assets;
    FModelCookedData payload;
    FMeshCookedData mesh;
    mesh.Name = "bad";
    mesh.MaterialIndex = 5; // no materials in payload
    mesh.IndexStride = 2;
    mesh.IndexCount = 0;
    AddPositionStream(mesh, 3);
    payload.Meshes.push_back(eastl::move(mesh));
    EXPECT_FALSE(ValidateModel(payload, "test"));
}

TEST(AssetValidate, RejectsMismatchedIndexData)
{
    using namespace Assets;
    FModelCookedData payload;
    FMeshCookedData mesh;
    mesh.Name = "bad";
    mesh.MaterialIndex = -1;
    mesh.IndexStride = 2;
    mesh.IndexCount = 10;
    mesh.IndexData.resize(8); // 8 != 2*10
    AddPositionStream(mesh, 10);
    payload.Meshes.push_back(eastl::move(mesh));
    EXPECT_FALSE(ValidateModel(payload, "test"));
}

TEST(AssetValidate, RejectsIndexOutOfRange)
{
    using namespace Assets;
    FModelCookedData payload;
    FMeshCookedData mesh;
    mesh.Name = "bad";
    mesh.MaterialIndex = -1;
    mesh.IndexStride = 2;
    mesh.IndexCount = 1;
    mesh.IndexData.resize(2);
    reinterpret_cast<uint16_t*>(mesh.IndexData.data())[0] = 99; // >= VertexCount
    AddPositionStream(mesh, 3);
    payload.Meshes.push_back(eastl::move(mesh));
    EXPECT_FALSE(ValidateModel(payload, "test"));
}

TEST(AssetValidate, RejectsEmptyKeyframeChannel)
{
    using namespace Assets;
    FModelCookedData payload;
    FAssetNode node;
    node.Name = "n";
    payload.Nodes.push_back(eastl::move(node));
    payload.RootNodes.push_back(0);
    FAssetAnimation anim;
    anim.Name = "anim";
    FAssetAnimationChannel channel;
    channel.TargetNode = 0;
    channel.Mode = 0;
    anim.Channels.push_back(eastl::move(channel));
    payload.Animations.push_back(eastl::move(anim));
    EXPECT_FALSE(ValidateModel(payload, "test"));
}

TEST(AssetValidate, RejectsNodeCycle)
{
    using namespace Assets;
    FModelCookedData payload;
    FAssetNode a, b;
    a.Name = "a";
    a.Parent = 1;
    a.Children.push_back(1);
    b.Name = "b";
    b.Parent = 0;
    b.Children.push_back(0);
    payload.Nodes.push_back(eastl::move(a));
    payload.Nodes.push_back(eastl::move(b));
    payload.RootNodes.push_back(0);
    EXPECT_FALSE(ValidateModel(payload, "test"));
}

TEST(AssetPath, TextureSourceRoundtrip)
{
    using namespace Assets;
    const eastl::string cooked = Assets::FAssetPathUtils::MakeCookedTexturePath("textures/albedo.png", ETextureUsage::BaseColor);
    EXPECT_EQ(cooked, "textures/albedo.png.BaseColor.vasset");
    EXPECT_EQ(FAssetPathUtils::GetTextureSourcePath(cooked), "textures/albedo.png");
    EXPECT_EQ(FAssetPathUtils::MakeCookedModelPath("Models/Box/glTF-Embedded/Box.gltf"), "Models/Box/glTF-Embedded/Box.gltf.vasset");
    EXPECT_EQ(FAssetPathUtils::MakeCookedModelPath("../Assets/Models/Box/glTF-Embedded/Box.gltf"), "../Content/Models/Box/glTF-Embedded/Box.gltf.vasset");
    EXPECT_EQ(FAssetPathUtils::MakeCookedModelPath("D:/x/y/Sponza.gltf"), "D:/x/y/Sponza.gltf.vasset");
    EXPECT_EQ(FAssetPathUtils::GetTextureSourcePath("textures/albedo.2k.png.BaseColor.vasset"), "textures/albedo.2k.png");
    EXPECT_EQ(FAssetPathUtils::GetTextureSourcePath("textures/albedo.Normal.png.Normal.vasset"), "textures/albedo.Normal.png");
}

TEST(AssetValidate, RejectsOversizedBlob)
{
    using namespace Assets;
    FModelCookedData payload;
    FMeshCookedData mesh;
    mesh.Name = "huge";
    mesh.MaterialIndex = -1;
    mesh.IndexStride = 2;
    mesh.IndexCount = 0;
    FMeshCookedData::FStream pos;
    pos.Type = EVertexStreamType::Position;
    pos.Stride = sizeof(float3);
    pos.Data.resize(65ull * 1024ull * 1024ull); // 65MB > 64MB budget (#10)
    mesh.VertexCount = static_cast<uint32_t>(pos.Data.size() / sizeof(float3));
    mesh.Streams.push_back(eastl::move(pos));
    payload.Meshes.push_back(eastl::move(mesh));
    EXPECT_FALSE(ValidateModel(payload, "test"));
}

// ----------- AssetAsync (Slice 4) ---------- //
#include "AssetManager/AssetManager.hpp"
#include <enkiTS/TaskScheduler.h>
#include <rpmalloc/rpmalloc.h>

namespace
{
    const char* kAsyncSrc = "assetcook_async_src.gltf";
    const char* kAsyncSrc2 = "assetcook_async_src2.gltf";

    void ConfigureAndInitTestScheduler(enki::TaskScheduler& ts)
    {
        enki::TaskSchedulerConfig tsConfig;
        tsConfig.profilerCallbacks.threadStart = [](uint32_t) { rpmalloc_thread_initialize(); };
        tsConfig.profilerCallbacks.threadStop = [](uint32_t) { rpmalloc_thread_finalize(1); };
        ts.Initialize(tsConfig);
    }

    void CopyBoxSource(const char* dst)
    {
        std::ifstream src(kBoxSrc, std::ios::binary);
        std::ofstream copy(dst, std::ios::binary);
        copy << src.rdbuf();
    }
}

TEST(AssetAsync, LoadsModelOnWorkerAndDrainsOnMainThread)
{
    using namespace Assets;
    CopyBoxSource(kAsyncSrc);

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    bool fired = false;
    bool success = false;
    uint32_t meshCount = 0;
    manager.RequestModel(kAsyncSrc, 
    [&](FModelLoadResult&& r)
    {
        fired = true;
        success = r.bSuccess;
        if (r.Payload) meshCount = (uint32_t)r.Payload->Meshes.size(); 
    });

    ts.WaitforAll();           // worker completes and pushes the ready result
    manager.DrainReadyQueue(); // main thread fires the callback
    EXPECT_TRUE(fired);
    EXPECT_TRUE(success);
    EXPECT_EQ(meshCount, 1u); // Box: 1 mesh
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

TEST(AssetAsync, DeferredInFlightRequestSharesPayload)
{
    using namespace Assets;
    CopyBoxSource(kAsyncSrc2);

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    FModelLoadResult r1;
    FModelLoadResult r2;
    manager.RequestModel(kAsyncSrc2, [&](FModelLoadResult&& r)
                         { r1 = eastl::move(r); });
    manager.RequestModel(kAsyncSrc2, [&](FModelLoadResult&& r)
                         { r2 = eastl::move(r); }); // in-flight dedup

    ts.WaitforAll();
    manager.DrainReadyQueue();
    EXPECT_TRUE(r1.bSuccess);
    EXPECT_TRUE(r2.bSuccess);
    ASSERT_TRUE(r1.Payload && r2.Payload);
    EXPECT_EQ(r1.Payload.get(), r2.Payload.get()); // same payload, no double cook / read
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

TEST(AssetAsync, CacheHitCompletesWithoutWorker)
{
    using namespace Assets;
    CopyBoxSource(kAsyncSrc);

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    FModelLoadResult r1;
    FModelLoadResult r2;
    manager.RequestModel(kAsyncSrc, [&](FModelLoadResult&& r)
                         { r1 = eastl::move(r); });
    ts.WaitforAll(); // worker done, entry cached, result queued
    manager.RequestModel(kAsyncSrc, [&](FModelLoadResult&& r)
                         { r2 = eastl::move(r); }); // cache hit
    manager.DrainReadyQueue();

    EXPECT_TRUE(r1.bSuccess);
    EXPECT_TRUE(r2.bSuccess);
    ASSERT_TRUE(r1.Payload && r2.Payload);
    EXPECT_EQ(r1.Payload.get(), r2.Payload.get());
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

TEST(AssetAsync, FailedLoadReportsFailure)
{
    using namespace Assets;
    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    bool fired = false;
    bool success = true;
    manager.RequestModel("no_such_model_asset.gltf", [&](FModelLoadResult&& r)
                         {
        fired = true;
        success = r.bSuccess; });

    ts.WaitforAll();
    manager.DrainReadyQueue();
    EXPECT_TRUE(fired);
    EXPECT_FALSE(success);
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

// ---- Slice 5: texture cook / read / validate / async ----
namespace
{
    const char* kBoxScreenshotPng = "../Assets/Models/Box/screenshot/screenshot.png";

    uint64_t ExpectedTexturePayloadSize(const Assets::FTextureCookedData& payload)
    {
        uint64_t total = 0;
        const uint32_t blockW = Assets::GetTextureFormatBlockWidth(payload.Format);
        const uint32_t blockH = Assets::GetTextureFormatBlockHeight(payload.Format);
        for (uint32_t mip = 0; mip < payload.MipLevels; ++mip)
        {
            total += Assets::GetTextureFormatMipSize(payload.Format,
                                                     Assets::GetTextureMipDim(payload.Width, mip, blockW),
                                                     Assets::GetTextureMipDim(payload.Height, mip, blockH),
                                                     payload.Depth);
        }
        return total * payload.ArraySize;
    }
} // namespace

TEST(AssetTexture, CookAndRoundtrip)
{
    using namespace Assets;
    const char* kDst = "assetcook_tex_test.BaseColor.vasset";

    ASSERT_TRUE(FVultanaCooker::CookTexture(kBoxScreenshotPng, kDst));

    FVAssetHeader header;
    FTextureCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadTexture(kDst, header, payload));
    EXPECT_EQ(header.Type, EAssetType::Texture);
    EXPECT_GT(payload.Width, 0u);
    EXPECT_GT(payload.Height, 0u);
    EXPECT_GT(payload.MipLevels, 1u); // full mip chain (BC floor at 4x4)
    EXPECT_EQ(payload.Depth, 1u);
    EXPECT_EQ(payload.ArraySize, 1u);
    EXPECT_EQ(payload.Type, ETextureType::Texture2D);
#if VULTANA_HAS_ISPC_TEXCOMP
    EXPECT_EQ(payload.Format, ETextureFormat::BC7SRGB); // BaseColor -> BC7 sRGB
#else
    EXPECT_EQ(payload.Format, ETextureFormat::RGBA8SRGB); // fallback without ISPC
#endif
    // Mip data must exactly match the UploadTexture byte layout.
    EXPECT_EQ(payload.MipData.size(), ExpectedTexturePayloadSize(payload));

    std::remove(kDst);
}

TEST(AssetTexture, RejectsMissingSource)
{
    using namespace Assets;
    EXPECT_FALSE(FVultanaCooker::CookTexture("no_such_texture_source.png", "assetcook_tex_missing.BaseColor.vasset"));
}

TEST(AssetTexture, ValidateRejectsBadMipData)
{
    using namespace Assets;
    FTextureCookedData payload;
    payload.Width = 4;
    payload.Height = 4;
    payload.Format = ETextureFormat::BC7UNORM; // 4x4 BC7 = 16 bytes
    payload.MipData.assign(16, 0);
    EXPECT_TRUE(ValidateTexture(payload, "test"));

    payload.MipData.assign(15, 0); // one byte short
    EXPECT_FALSE(ValidateTexture(payload, "test"));

    payload.MipData.assign(17, 0); // one byte over
    EXPECT_FALSE(ValidateTexture(payload, "test"));

    payload.MipData.assign(16, 0);
    payload.MipLevels = 2; // second mip (clamped 4x4 = 16B) missing
    EXPECT_FALSE(ValidateTexture(payload, "test"));
}

TEST(AssetTexture, TruncatedCookedTextureFails)
{
    using namespace Assets;
    const char* kDst = "assetcook_tex_test.BaseColor.vasset";
    ASSERT_TRUE(FVultanaCooker::CookTexture(kBoxScreenshotPng, kDst));
    TruncateFile(kDst, 48 + 2 * 24); // header + section table only; payload gone

    FVAssetHeader header;
    FTextureCookedData payload;
    EXPECT_FALSE(FAssetReader::ReadTexture(kDst, header, payload));
    std::remove(kDst);
}

TEST(AssetTexture, ProductPathRoundtrip)
{
    using namespace Assets;
    eastl::string srcRel;
    ETextureUsage usage = ETextureUsage::Cubemap; // any valid sentinel (B6: no ETextureUsage::Unknown)

    EXPECT_TRUE(FAssetPathUtils::ParseTextureProductPath(
        "../Content/Models/Box/glTF-Embedded/Default_albedo.jpg.BaseColor.vasset", srcRel, usage));
    EXPECT_EQ(srcRel, "Models/Box/glTF-Embedded/Default_albedo.jpg");
    EXPECT_EQ(usage, ETextureUsage::BaseColor);

    EXPECT_TRUE(FAssetPathUtils::ParseTextureProductPath(
        "Models/Box/tex.png.Normal.vasset", srcRel, usage)); // relative form
    EXPECT_EQ(srcRel, "Models/Box/tex.png");
    EXPECT_EQ(usage, ETextureUsage::Normal);

    EXPECT_FALSE(FAssetPathUtils::ParseTextureProductPath("Models/Box/foo.gltf.vasset", srcRel, usage)); // not a texture product
    EXPECT_FALSE(FAssetPathUtils::ParseTextureProductPath("Models/Box/foo.png", srcRel, usage));

    EXPECT_TRUE(FAssetPathUtils::IsDDSSource("Textures/foo.dds"));
    EXPECT_TRUE(FAssetPathUtils::IsDDSSource("Textures/foo.DDS"));
    EXPECT_FALSE(FAssetPathUtils::IsDDSSource("Textures/foo.png"));
    EXPECT_FALSE(FAssetPathUtils::IsDDSSource("Textures/foo.ddsx"));
}

TEST(AssetAsync, TextureLoadsOnWorkerAndDrainsOnMainThread)
{
    using namespace Assets;
    // Product path MUST correspond to a real source: the worker derives the source from the
    // product path (ParseTextureProductPath), so an arbitrary name would cook-fail.
    const char* kRelSrc = "Models/Box/screenshot/screenshot.png";
    const eastl::string kProduct = FAssetPathUtils::GetContentRootPath() + FAssetPathUtils::MakeCookedTexturePath(kRelSrc, ETextureUsage::BaseColor);
    ASSERT_TRUE(FVultanaCooker::CookTexture(FAssetPathUtils::GetAssetsRootPath() + kRelSrc, kProduct)); // pre-cook so the worker only reads

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    bool fired = false;
    bool success = false;
    uint32_t mipLevels = 0;
    manager.RequestTexture(kProduct,
    [&](FTextureLoadResult&& r)
    {
        fired = true;
        success = r.bSuccess;
        if (r.Payload) mipLevels = r.Payload->MipLevels;
    });

    ts.WaitforAll();
    manager.DrainReadyQueue(); // main thread fires the callback (GPU create is null-guarded)
    EXPECT_TRUE(fired);
    EXPECT_TRUE(success);
    EXPECT_GT(mipLevels, 0u);
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();

    std::remove(kProduct.c_str());
}

TEST(AssetAsync, FailedTextureLoadReportsFailure)
{
    using namespace Assets;
    const eastl::string kProduct = FAssetPathUtils::GetContentRootPath() + "no_such_texture_asset.BaseColor.vasset";

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    bool fired = false;
    bool success = true;
    manager.RequestTexture(kProduct,
    [&](FTextureLoadResult&& r)
    {
        fired = true;
        success = r.bSuccess;
    });

    ts.WaitforAll();
    manager.DrainReadyQueue();
    EXPECT_TRUE(fired);
    EXPECT_FALSE(success);
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

TEST(AssetTexture, MipDimSyncsWithRHI)
{
    // Pins Assets::GetTextureMipDim (cook/validate) byte-identical to RHI::GetFormatMipDim
    // (UploadTexture/GetRowPitch) — the two must never drift (B9).
    for (uint32_t base = 1; base <= 4096; base += 7)
    {
        for (uint32_t mip = 0; mip < 12; ++mip)
        {
            EXPECT_EQ(Assets::GetTextureMipDim(base, mip, 1u), RHI::GetFormatMipDim(base, mip, 1u));
            EXPECT_EQ(Assets::GetTextureMipDim(base, mip, 4u), RHI::GetFormatMipDim(base, mip, 4u));
        }
    }
}

TEST(AssetTexture, BC5NormalIsNotDegenerate)
{
    // B4 regression guard: a BC5 cook of a gradient source must not produce an all-zeros /
    // all-constant block (the historical RGBA8-fed-blocks bug). Writes a 4x4 RGBA gradient
    // PNG via the test fixture, cooks as Normal (BC5), and checks the block bytes differ.
    using namespace Assets;
    const char* kPng = "assetcook_bc5_grad.png";
    const char* kDst = "assetcook_bc5_grad.Normal.vasset";
    {
        const int w = 4, h = 4;
        eastl::vector<uint8_t> rgba((size_t)w * h * 4);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                rgba[((size_t)y * w + x) * 4 + 0] = (uint8_t)(x * 64); // R ramps
                rgba[((size_t)y * w + x) * 4 + 1] = (uint8_t)(y * 64); // G ramps
                rgba[((size_t)y * w + x) * 4 + 2] = 128;
                rgba[((size_t)y * w + x) * 4 + 3] = 255;
            }
        stbi_write_png(kPng, w, h, 4, rgba.data(), w * 4);
    }
#if VULTANA_HAS_ISPC_TEXCOMP
    ASSERT_TRUE(FVultanaCooker::CookTexture(kPng, kDst));
    FVAssetHeader header;
    FTextureCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadTexture(kDst, header, payload));
    EXPECT_EQ(payload.Format, ETextureFormat::BC5UNORM);
    ASSERT_GE(payload.MipData.size(), 16u); // one 4x4 BC5 block = 16 bytes at minimum
    bool bAnyNonZero = false;
    for (uint8_t b : payload.MipData)
        if (b != 0)
        {
            bAnyNonZero = true;
            break;
        }
    EXPECT_TRUE(bAnyNonZero); // packed gradient must survive
    std::remove(kDst);
#endif
    std::remove(kPng);
}

namespace
{
    using namespace Assets;

    const char* kObjSrc = "assimp_test_cube.obj";
    const char* kObjMtlSrc = "assimp_test_cube.mtl";
    const char* kDaeSrc = "assimp_test_tri.dae";

    void WriteObjCube(const char* objPath, const char* mtlPath)
    {
        std::ofstream mtl(mtlPath, std::ios::binary | std::ios::trunc);
        mtl << "newmtl Mat_BaseColor\n";
        mtl << "Kd 0.8 0.2 0.2\n";
        mtl << "map_Kd cube_basecolor.png\n";
        mtl.close();

        // 8 vertices, 24 UVs (each face gets its own 0..1 square so every (pos,uv) pair is
        // unique -> JoinIdenticalVertices yields exactly 24 vertices, deterministic).
        std::ofstream obj(objPath, std::ios::binary | std::ios::trunc);
        obj << "mtllib assimp_test_cube.mtl\n";
        obj << "o Cube\n";
        obj << "v -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\n";
        obj << "v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\n";
        for (int f = 0; f < 6; f++)
            obj << "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n";
        obj << "usemtl Mat_BaseColor\n";
        obj << "f 1/1 2/2 3/3\nf 1/1 3/3 4/4\n";       // +Z
        obj << "f 5/5 7/7 6/6\nf 5/5 8/8 7/7\n";       // -Z
        obj << "f 2/9 7/11 6/10\nf 2/9 3/12 7/11\n";   // +X
        obj << "f 1/13 8/15 5/14\nf 1/13 4/16 8/15\n"; // -X
        obj << "f 4/17 3/18 7/19\nf 4/17 7/19 8/20\n"; // +Y
        obj << "f 1/21 6/23 2/22\nf 1/21 5/24 6/23\n"; // -Y
        obj.close();
    }

    void WriteDaeTri(const char* daePath)
    {
        std::ofstream dae(daePath, std::ios::binary | std::ios::trunc);
        dae << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        dae << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n";
        dae << "  <asset><up_axis>Y_UP</up_axis></asset>\n";
        dae << "  <library_effects>\n";
        dae << "    <effect id=\"tri-fx\"><profile_COMMON>\n";
        dae << "      <newparam sid=\"tri-surface\"><surface type=\"2D\"><init_from>tri-img</init_from></surface></newparam>\n";
        dae << "      <newparam sid=\"tri-sampler\"><sampler2D><source>tri-surface</source></sampler2D></newparam>\n";
        dae << "      <technique sid=\"common\"><lambert>\n";
        dae << "        <diffuse><texture texture=\"tri-sampler\" texcoord=\"UVMap\"/></diffuse>\n";
        dae << "      </lambert></technique>\n";
        dae << "    </profile_COMMON></effect>\n";
        dae << "  </library_effects>\n";
        dae << "  <library_images>\n";
        dae << "    <image id=\"tri-img\" name=\"tri-img\"><init_from>tri_diffuse.png</init_from></image>\n";
        dae << "  </library_images>\n";
        dae << "  <library_materials>\n";
        dae << "    <material id=\"tri-mat\" name=\"TriMat\"><instance_effect url=\"#tri-fx\"/></material>\n";
        dae << "  </library_materials>\n";
        dae << "  <library_geometries>\n";
        dae << "    <geometry id=\"tri-geom\" name=\"TriGeom\"><mesh>\n";
        dae << "      <source id=\"tri-pos\">\n";
        dae << "        <float_array id=\"tri-pos-array\" count=\"9\">0 0 0 1 0 0 0 1 0</float_array>\n";
        dae << "        <technique_common><accessor source=\"#tri-pos-array\" count=\"3\" stride=\"3\"><param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/></accessor></technique_common>\n";
        dae << "      </source>\n";
        dae << "      <source id=\"tri-tex\">\n";
        dae << "        <float_array id=\"tri-tex-array\" count=\"6\">0 0 1 0 0 1</float_array>\n";
        dae << "        <technique_common><accessor source=\"#tri-tex-array\" count=\"3\" stride=\"2\"><param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/></accessor></technique_common>\n";
        dae << "      </source>\n";
        dae << "      <vertices id=\"tri-verts\"><input semantic=\"POSITION\" source=\"#tri-pos\"/></vertices>\n";
        dae << "      <polylist material=\"tri-mat\" count=\"1\">\n";
        dae << "        <input semantic=\"VERTEX\" source=\"#tri-verts\" offset=\"0\"/>\n";
        dae << "        <input semantic=\"TEXCOORD\" source=\"#tri-tex\" offset=\"1\" set=\"0\"/>\n";
        dae << "        <vcount>3</vcount>\n";
        dae << "        <p>0 0 1 1 2 2</p>\n";
        dae << "      </polylist>\n";
        dae << "    </mesh></geometry>\n";
        dae << "  </library_geometries>\n";
        dae << "  <library_animations>\n";
        dae << "    <animation id=\"tri-anim\" name=\"TriAnim\">\n";
        dae << "      <source id=\"tri-time\">\n";
        dae << "        <float_array id=\"tri-time-array\" count=\"2\">0 1</float_array>\n";
        dae << "        <technique_common><accessor source=\"#tri-time-array\" count=\"2\" stride=\"1\"><param name=\"TIME\" type=\"float\"/></accessor></technique_common>\n";
        dae << "      </source>\n";
        dae << "      <source id=\"tri-pos-anim\">\n";
        dae << "        <float_array id=\"tri-pos-anim-array\" count=\"6\">2 3 4 2 3 8</float_array>\n";
        dae << "        <technique_common><accessor source=\"#tri-pos-anim-array\" count=\"2\" stride=\"3\"><param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/></accessor></technique_common>\n";
        dae << "      </source>\n";
        dae << "      <sampler id=\"tri-sampler-anim\">\n";
        dae << "        <input semantic=\"INPUT\" source=\"#tri-time\"/>\n";
        dae << "        <input semantic=\"OUTPUT\" source=\"#tri-pos-anim\"/>\n";
        dae << "      </sampler>\n";
        dae << "      <channel source=\"#tri-sampler-anim\" target=\"tri-node/translation\"/>\n";
        dae << "    </animation>\n";
        dae << "  </library_animations>\n";
        dae << "  <library_visual_scenes>\n";
        dae << "    <visual_scene id=\"scene\" name=\"scene\">\n";
        dae << "      <node id=\"tri-node\" name=\"tri-node\" sid=\"tri-node\" type=\"NODE\">\n";
        dae << "        <translate sid=\"translation\">2 3 4</translate>\n";
        dae << "        <node id=\"tri-child\" name=\"tri-child\" sid=\"tri-child\" type=\"NODE\">\n";
        dae << "          <rotate sid=\"rotation\">0 1 0 90</rotate>\n";
        dae << "          <scale sid=\"scale\">1 2 3</scale>\n";
        dae << "          <instance_geometry url=\"#tri-geom\">\n";
        dae << "            <bind_material><technique_common><instance_material symbol=\"tri-mat\" target=\"#tri-mat\"/></technique_common></bind_material>\n";
        dae << "          </instance_geometry>\n";
        dae << "        </node>\n";
        dae << "      </node>\n";
        dae << "    </visual_scene>\n";
        dae << "  </library_visual_scenes>\n";
        dae << "  <scene><instance_visual_scene url=\"#scene\"/></scene>\n";
        dae << "</COLLADA>\n";
        dae.close();
    }

    template <typename T>
    bool HasStream(const T& mesh, EVertexStreamType type)
    {
        for (const auto& s : mesh.Streams)
        {
            if (s.Type == type)
                return true;
        }
        return false;
    }
} // namespace

TEST(AssetAssimp, CookObjCubeProducesExpectedMesh)
{
    using namespace Assets;
    WriteObjCube(kObjSrc, kObjMtlSrc);
    ASSERT_TRUE(FVultanaCooker::CookModel(kObjSrc, kCookDst, /*bSilent=*/true));

    FVAssetHeader header;
    FModelCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadModel(kCookDst, header, payload));

    ASSERT_EQ(payload.Meshes.size(), 1u);
    EXPECT_EQ(payload.Meshes[0].VertexCount, 24u); // 8 pos x 3 faces share no (pos,uv) pair
    EXPECT_EQ(payload.Meshes[0].IndexCount, 36u);
    EXPECT_EQ(payload.Meshes[0].IndexStride, 2u);
    EXPECT_FALSE(payload.Meshes[0].bSkeletal);
    EXPECT_GT(payload.Meshes[0].MeshletCount, 0u); // static mesh must have meshlets
    EXPECT_TRUE(HasStream(payload.Meshes[0], EVertexStreamType::Position));
    EXPECT_TRUE(HasStream(payload.Meshes[0], EVertexStreamType::TexCoord));
    EXPECT_TRUE(HasStream(payload.Meshes[0], EVertexStreamType::Normal));
    EXPECT_TRUE(HasStream(payload.Meshes[0], EVertexStreamType::Tangent));
    EXPECT_FLOAT_EQ(payload.Meshes[0].Center.x, 0.0f);
    EXPECT_FLOAT_EQ(payload.Meshes[0].Center.y, 0.0f);
    EXPECT_FLOAT_EQ(payload.Meshes[0].Center.z, 0.0f);
    EXPECT_NEAR(payload.Meshes[0].Radius, 1.73205f, 0.01f); // cube half-diagonal

    // Look the material up by name: OBJ importers historically also synthesize a
    // "DefaultMaterial" slot, so hardcoding index 0 would be brittle.
    ASSERT_GE(payload.Materials.size(), 1u);
    const auto* mat = &payload.Materials[0];
    for (const auto& m : payload.Materials)
    {
        if (m.Name == "Mat_BaseColor")
        {
            mat = &m;
            break;
        }
    }
    EXPECT_STREQ(mat->Name.c_str(), "Mat_BaseColor");
    EXPECT_EQ((int)mat->WorkFlow, (int)EMaterialWorkFlow::PBRMetallicRoughness);
    EXPECT_FLOAT_EQ(mat->AlbedoColor.x, 0.8f);
    EXPECT_FLOAT_EQ(mat->AlbedoColor.y, 0.2f);
    EXPECT_FLOAT_EQ(mat->AlbedoColor.z, 0.2f);
    ASSERT_EQ(mat->Textures.size(), 1u);
    EXPECT_EQ((int)mat->Textures[0].Usage, (int)ETextureUsage::BaseColor);
    EXPECT_TRUE(mat->Textures[0].bSRGB);
    EXPECT_STREQ(mat->Textures[0].RelativePath.c_str(), "cube_basecolor.png.BaseColor.vasset");

    ASSERT_EQ(payload.RootNodes.size(), 1u);
    ASSERT_EQ(payload.Nodes.size(), 1u);
    ASSERT_EQ(payload.Nodes[0].Primitives.size(), 1u);
    EXPECT_EQ(payload.Nodes[0].Primitives[0], 0);
    EXPECT_TRUE(payload.Nodes[0].bFrontFaceCCW); // identity world transform, CCW source faces

    std::remove(kObjSrc);
    std::remove(kObjMtlSrc);
}

TEST(AssetAssimp, CookDaeHierarchyTransformAndAnimation)
{
    using namespace Assets;
    WriteDaeTri(kDaeSrc);
    ASSERT_TRUE(FVultanaCooker::CookModel(kDaeSrc, kCookDst, /*bSilent=*/true));

    FVAssetHeader header;
    FModelCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadModel(kCookDst, header, payload));

    // Geometry: one triangle (Y_UP is the DAE default, so no up-axis conversion is applied).
    ASSERT_EQ(payload.Meshes.size(), 1u);
    EXPECT_EQ(payload.Meshes[0].IndexCount, 3u);
    EXPECT_EQ(payload.Meshes[0].VertexCount, 3u);
    EXPECT_EQ(payload.Meshes[0].IndexStride, 2u);
    EXPECT_TRUE(HasStream(payload.Meshes[0], EVertexStreamType::Position));
    EXPECT_TRUE(HasStream(payload.Meshes[0], EVertexStreamType::TexCoord));

    // Material + texture ref: lambert diffuse -> BaseColor slot.
    ASSERT_EQ(payload.Materials.size(), 1u);
    ASSERT_EQ(payload.Materials[0].Textures.size(), 1u);
    EXPECT_EQ((int)payload.Materials[0].Textures[0].Usage, (int)ETextureUsage::BaseColor);
    EXPECT_STREQ(payload.Materials[0].Textures[0].RelativePath.c_str(), "tri_diffuse.png.BaseColor.vasset");

    // Two-level hierarchy: tri-node (<translate>) -> tri-child (<rotate> + <scale> +
    // geometry). This exercises the Parent/Children back edges that AssetValidate checks —
    // a single-level fixture would silently pass with Children left unpopulated.
    ASSERT_EQ(payload.RootNodes.size(), 1u);
    ASSERT_EQ(payload.Nodes.size(), 2u);
    const FAssetNode* triNode = nullptr;
    const FAssetNode* triChild = nullptr;
    for (const FAssetNode& node : payload.Nodes)
    {
        if (node.Parent == -1)
            triNode = &node;
        else
            triChild = &node;
    }
    ASSERT_NE(triNode, nullptr);
    ASSERT_NE(triChild, nullptr);
    ASSERT_EQ(triNode->Children.size(), 1u);
    ASSERT_EQ(triNode->Children[0], 1); // back edge: tri-child is the only non-root node

    // tri-node: <translate>2 3 4</translate> must cook to (2, 3, -4). This pins the
    // aiMatrix4x4 (row-major) -> linalg (column-major) conversion end to end.
    EXPECT_FLOAT_EQ(triNode->Translation.x, 2.0f);
    EXPECT_FLOAT_EQ(triNode->Translation.y, 3.0f);
    EXPECT_FLOAT_EQ(triNode->Translation.z, -4.0f);

    // tri-child: identity translation, Y-axis 90-degree rotation + non-uniform scale. The
    // rotation pins the ToLinalg linear part — a transposed mapping would produce the
    // inverse rotation (and mis-order the columns), which these values would catch.
    ASSERT_EQ(triChild->Primitives.size(), 1u);
    EXPECT_EQ(triChild->Primitives[0], 0);
    EXPECT_NEAR(triChild->Translation.x, 0.0f, 1e-4f);
    EXPECT_NEAR(triChild->Translation.y, 0.0f, 1e-4f);
    EXPECT_NEAR(triChild->Translation.z, 0.0f, 1e-4f);
    EXPECT_NEAR(triChild->Rotation.x, 0.0f, 1e-3f);
    EXPECT_NEAR(triChild->Rotation.y, 0.70711f, 1e-3f); // RH Y-90 deg -> (x,y,z,w) = (0, ~.7071, 0, ~.7071)
    EXPECT_NEAR(triChild->Rotation.z, 0.0f, 1e-3f);
    EXPECT_NEAR(triChild->Rotation.w, -0.70711f, 1e-3f); // z/w flipped by the RH->LH rule
    EXPECT_NEAR(triChild->Scale.x, 1.0f, 1e-3f);
    EXPECT_NEAR(triChild->Scale.y, 2.0f, 1e-3f);
    EXPECT_NEAR(triChild->Scale.z, 3.0f, 1e-3f);
    EXPECT_TRUE(triChild->bFrontFaceCCW); // det(R_y(90) * diag(1,2,3)) = +6 > 0

    // Animation: 1 clip, equal-length T/R/S tracks (assimp's ColladaLoader resamples the
    // full node transform at every key time, so unanimated rotation/scale also get keys),
    // times in seconds (DAE ms / 1000). Keyframe values stay in the source (RH) convention —
    // the runtime flips at sample time.
    ASSERT_EQ(payload.Animations.size(), 1u);
    ASSERT_EQ(payload.Animations[0].Channels.size(), 3u); // translation + static rotation + static scale
    const FAssetAnimationChannel* transCh = nullptr;
    for (const FAssetAnimationChannel& c : payload.Animations[0].Channels)
    {
        if (c.Mode == 0u)
        {
            transCh = &c;
            break;
        }
    }
    ASSERT_NE(transCh, nullptr);
    ASSERT_EQ(transCh->KeyFrames.size(), 2u);
    EXPECT_FLOAT_EQ(transCh->KeyFrames[0].first, 0.0f);
    EXPECT_FLOAT_EQ(transCh->KeyFrames[1].first, 1.0f);
    EXPECT_FLOAT_EQ(transCh->KeyFrames[1].second.x, 2.0f);
    EXPECT_FLOAT_EQ(transCh->KeyFrames[1].second.y, 3.0f);
    EXPECT_FLOAT_EQ(transCh->KeyFrames[1].second.z, 8.0f); // no RH->LH flip at cook time
    EXPECT_FLOAT_EQ(payload.Animations[0].TimeDuration, 1.0f);

    std::remove(kDaeSrc);
}

TEST(AssetAssimp, CookObjIsDeterministic)
{
    using namespace Assets;
    WriteObjCube(kObjSrc, kObjMtlSrc);
    // Same byte-compare idiom as AssetCook.CookIsDeterministic: assimp has no fixed pipeline
    // order documented across versions, so determinism gets an explicit guard.
    ASSERT_TRUE(FVultanaCooker::CookModel(kObjSrc, kCookDst, /*bSilent=*/true));
    std::ifstream first(kCookDst, std::ios::binary | std::ios::ate);
    const size_t size1 = static_cast<size_t>(first.tellg());
    eastl::vector<uint8_t> buf1(size1);
    first.seekg(0);
    first.read(reinterpret_cast<char*>(buf1.data()), size1);
    first.close();

    ASSERT_TRUE(FVultanaCooker::CookModel(kObjSrc, kCookDst, /*bSilent=*/true));
    std::ifstream second(kCookDst, std::ios::binary | std::ios::ate);
    const size_t size2 = static_cast<size_t>(second.tellg());
    eastl::vector<uint8_t> buf2(size2);
    second.seekg(0);
    second.read(reinterpret_cast<char*>(buf2.data()), size2);
    second.close();

    EXPECT_EQ(size1, size2);
    if (size1 == size2)
    {
        EXPECT_EQ(memcmp(buf1.data(), buf2.data(), size1), 0);
    }
}

TEST(AssetAssimp, GLTFPathStillDispatchesToCGLTF)
{
    using namespace Assets;
    // Dispatch regression: .gltf must keep routing to the cgltf importer now that CookModel
    // dispatches on extension. The asserted topology is the same known-good cgltf output that
    // AssetCook.CookBoxProducesExpectedTopology pins; the assimp path would emit 1 node for
    // this file (its identity root is skipped), so Nodes == 2 discriminates the dispatch.
    ASSERT_TRUE(FVultanaCooker::CookModel(kBoxSrc, kCookDst, /*bSilent=*/true));

    FVAssetHeader header;
    FModelCookedData payload;
    ASSERT_TRUE(FAssetReader::ReadModel(kCookDst, header, payload));
    EXPECT_EQ(payload.Meshes.size(), 1u);
    EXPECT_EQ(payload.Nodes.size(), 2u);
    EXPECT_EQ(payload.Materials.size(), 1u);
    EXPECT_EQ(payload.Meshes[0].MaterialIndex, 0);
    EXPECT_GT(payload.Meshes[0].MeshletCount, 0u);
    EXPECT_FALSE(payload.Meshes[0].bSkeletal);
}

// ---- Slice 7: Hot Reload----
TEST(AssetAsync, ReloadReCookesAndEvictsCache)
{
    using namespace Assets;
    CopyBoxSource(kAsyncSrc);

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    FModelLoadResult first;
    manager.RequestModel(kAsyncSrc, [&](FModelLoadResult&& r)
                         { first = eastl::move(r); });
    ts.WaitforAll();
    manager.DrainReadyQueue();
    ASSERT_TRUE(first.bSuccess);
    ASSERT_TRUE(first.Payload);

    FModelLoadResult reloaded;
    manager.RequestModelReload(kAsyncSrc, 
        [&](FModelLoadResult&& r)
        { reloaded = eastl::move(r); });
    ts.WaitforAll();
    manager.DrainReadyQueue();

    EXPECT_TRUE(reloaded.bSuccess);
    ASSERT_TRUE(reloaded.Payload);
    EXPECT_EQ(reloaded.Payload->Meshes.size(), 1u);

    EXPECT_NE(reloaded.Payload.get(), first.Payload.get());
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

TEST(AssetAsync, ReloadWhileLoadingRespawnsForceCook)
{
    using namespace Assets;
    CopyBoxSource(kAsyncSrc2);

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    FModelLoadResult load;
    FModelLoadResult reload;
    manager.RequestModel(kAsyncSrc2, 
        [&](FModelLoadResult&& r)
        { load = eastl::move(r); });

    manager.RequestModelReload(kAsyncSrc2, 
        [&](FModelLoadResult&& r) 
        { reload = eastl::move(r); });

    ts.WaitforAll();
    manager.DrainReadyQueue();

    EXPECT_TRUE(load.bSuccess);
    EXPECT_TRUE(reload.bSuccess);
    ASSERT_TRUE(load.Payload && reload.Payload);
    EXPECT_NE(reload.Payload.get(), load.Payload.get());
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

TEST(AssetAsync, ReloadFailsWhenSourceMissing)
{
    using namespace Assets;
    CopyBoxSource(kAsyncSrc);

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    FModelLoadResult first;
    manager.RequestModel(kAsyncSrc, [&](FModelLoadResult&& r)
                         { first = eastl::move(r); });
    ts.WaitforAll();
    manager.DrainReadyQueue();
    ASSERT_TRUE(first.bSuccess);

    std::remove(kAsyncSrc);
    bool reloadFired = false;
    bool reloadSuccess = true;
    manager.RequestModelReload(kAsyncSrc, 
    [&](FModelLoadResult&& r)
    {
        reloadFired = true;
        reloadSuccess = r.bSuccess; 
    });

    ts.WaitforAll();
    manager.DrainReadyQueue();
    EXPECT_TRUE(reloadFired);
    EXPECT_FALSE(reloadSuccess);
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

TEST(AssetAsync, TwoReloadsWhileLoadingDoNotLeakPendingCount)
{
    using namespace Assets;
    CopyBoxSource(kAsyncSrc2);

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    FModelLoadResult load, r1, r2;
    manager.RequestModel(kAsyncSrc2, 
        [&](FModelLoadResult&& r)
        { load = eastl::move(r); });
    manager.RequestModelReload(kAsyncSrc2, 
        [&](FModelLoadResult&& r)
        { r1 = eastl::move(r); });
    manager.RequestModelReload(kAsyncSrc2, 
        [&](FModelLoadResult&& r)
        { r2 = eastl::move(r); });

    for (int i = 0; i < 4; ++i)
    {
        ts.WaitforAll();
        manager.DrainReadyQueue();
    }

    EXPECT_TRUE(load.bSuccess);
    EXPECT_TRUE(r1.bSuccess);
    EXPECT_TRUE(r2.bSuccess);
    EXPECT_FALSE(manager.HasPendingRequests());
    EXPECT_EQ(manager.GetPendingRequestCount(), 0u);
    ts.WaitforAll();
}

// ---- Slice 7 Part B: Texture Hot Reload ----
TEST(AssetAsync, ReloadTextureReCookesChangedSource)
{
    using namespace Assets;
    const char* kRelSrc = "Models/ReloadTest/tex_reload.png";
    const eastl::string kSrc = FAssetPathUtils::GetAssetsRootPath() + kRelSrc;
    const eastl::string kProduct = FAssetPathUtils::GetContentRootPath() + FAssetPathUtils::MakeCookedTexturePath(kRelSrc, ETextureUsage::BaseColor);
    const eastl::string kSrcDir = FAssetPathUtils::GetAssetsRootPath() + "Models/ReloadTest";
    const eastl::string kProdDir = FAssetPathUtils::GetContentRootPath() + "Models/ReloadTest";
    
    struct FCleanup
    {
        eastl::string Src, Product, SrcDir, ProdDir;
        ~FCleanup()
        {
            std::remove(Product.c_str());
            std::remove(Src.c_str());
            std::error_code ec;
            std::filesystem::remove_all(std::filesystem::path(SrcDir.c_str()), ec);
            std::filesystem::remove_all(std::filesystem::path(ProdDir.c_str()), ec);
        }
    } cleanup{kSrc, kProduct, kSrcDir, kProdDir};
    std::filesystem::create_directories(std::filesystem::path(kSrcDir.c_str()));
    {
        const int w = 4, h = 4;
        unsigned char px[4 * 16];
        for (int i = 0; i < 16; ++i)
        {
            px[i * 4 + 0] = 255;
            px[i * 4 + 1] = 0;
            px[i * 4 + 2] = 0;
            px[i * 4 + 3] = 255;
        }
        ASSERT_TRUE(stbi_write_png(kSrc.c_str(), w, h, 4, px, w * 4));
    }
    ASSERT_TRUE(FVultanaCooker::CookTexture(kSrc, kProduct));

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    FTextureLoadResult first;
    manager.RequestTexture(kProduct, [&](FTextureLoadResult&& r)
                           { first = eastl::move(r); });
    ts.WaitforAll();
    manager.DrainReadyQueue();
    ASSERT_TRUE(first.bSuccess && first.Payload);

    {
        const int w = 4, h = 4;
        unsigned char px[4 * 16];
        for (int i = 0; i < 16; ++i)
        {
            px[i * 4 + 0] = 0;
            px[i * 4 + 1] = 0;
            px[i * 4 + 2] = 255;
            px[i * 4 + 3] = 255;
        } // v2: 蓝
        ASSERT_TRUE(stbi_write_png(kSrc.c_str(), w, h, 4, px, w * 4));
    }
    FTextureLoadResult reloaded;
    manager.RequestTextureReload(kProduct, [&](FTextureLoadResult&& r)
                                 { reloaded = eastl::move(r); });
    ts.WaitforAll();
    manager.DrainReadyQueue();

    EXPECT_TRUE(reloaded.bSuccess);
    ASSERT_TRUE(reloaded.Payload);
    EXPECT_NE(reloaded.Payload.get(), first.Payload.get());

    EXPECT_NE(reloaded.ContentHash, 0ull);
    EXPECT_NE(first.ContentHash, 0ull);
    EXPECT_NE(reloaded.ContentHash, first.ContentHash);

    ASSERT_FALSE(reloaded.Payload->MipData.empty());
    ASSERT_EQ(reloaded.Payload->MipData.size(), first.Payload->MipData.size());
    EXPECT_NE(memcmp(reloaded.Payload->MipData.data(), first.Payload->MipData.data(), reloaded.Payload->MipData.size()), 0);
    EXPECT_FALSE(manager.HasPendingRequests());
    ts.WaitforAll();
}

TEST(AssetAsync, TwoTextureReloadsWhileLoadingDoNotLeakPendingCount)
{
    using namespace Assets;
    const char* kRelSrc = "Models/Box/screenshot/screenshot.png";
    const eastl::string kProduct = FAssetPathUtils::GetContentRootPath() + FAssetPathUtils::MakeCookedTexturePath(kRelSrc, ETextureUsage::Normal);
    ASSERT_TRUE(FVultanaCooker::CookTexture(FAssetPathUtils::GetAssetsRootPath() + kRelSrc, kProduct));

    enki::TaskScheduler ts;
    ConfigureAndInitTestScheduler(ts);
    FAssetManager manager(nullptr, &ts);

    FTextureLoadResult load, t1, t2;
    manager.RequestTexture(kProduct, [&](FTextureLoadResult&& r)
                           { load = eastl::move(r); });
    manager.RequestTextureReload(kProduct, [&](FTextureLoadResult&& r)
                                 { t1 = eastl::move(r); });
    manager.RequestTextureReload(kProduct, [&](FTextureLoadResult&& r)
                                 { t2 = eastl::move(r); });

    for (int i = 0; i < 4; ++i) // 容纳重生 worker 的额外轮次(§R1 建议 3)
    {
        ts.WaitforAll();
        manager.DrainReadyQueue();
    }

    EXPECT_TRUE(load.bSuccess);
    EXPECT_TRUE(t1.bSuccess);
    EXPECT_TRUE(t2.bSuccess);
    EXPECT_FALSE(manager.HasPendingRequests());
    EXPECT_EQ(manager.GetPendingRequestCount(), 0u);
    ts.WaitforAll();

    std::remove(kProduct.c_str());
}

TEST(AssetPath, MakeEngineAssetPathNormalizesToSchemeForm)
{
    using namespace Assets;
    const eastl::string rel = "Models/Box/glTF-Embedded/Box.gltf";
    const eastl::string scheme = FAssetPathUtils::GetAssetsRootPath() + rel; // "../Assets/Models/..."
    const eastl::string abs = std::filesystem::absolute(scheme.c_str()).string().c_str();
    EXPECT_EQ(FAssetPathUtils::MakeEngineAssetPath(abs), scheme);
    EXPECT_EQ(FAssetPathUtils::MakeEngineAssetPath(scheme), scheme);
    const eastl::string outside = FAssetPathUtils::MakeEngineAssetPath("../External/foo.gltf");
    EXPECT_NE(outside, "External/foo.gltf");
    EXPECT_TRUE(std::filesystem::path(outside.c_str()).is_absolute());
    EXPECT_NE(outside.find("/External/foo.gltf"), eastl::string::npos);
}