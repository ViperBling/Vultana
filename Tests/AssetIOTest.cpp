#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetCooker/AssetReader.hpp"
#include "AssetCooker/AssetWriter.hpp"
#include "AssetCooker/AssetValidate.hpp"
#include "AssetCooker/VultanaCooker.hpp"

#include "RHI/RHI.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
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