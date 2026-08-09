#pragma once

#include "Utilities/Math.hpp"
#include "Common/Meshlet.hlsli"

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/utility.h>

#include <cstdint>

namespace Assets
{
    // ---------------------------------------------------------------------------
    // Serialized record-size lower bound (compile-time). The .vasset payload records
    // are written field-by-field (FBinaryWriter); the *minimum* bytes a record can
    // occupy is the sum of its fixed-size fields, with variable-length fields
    // (strings / vectors) counted as their 4B length/count prefix only. The reader
    // uses this as a sanity divisor so a corrupt count can't drive an over-allocation.
    // Field lists below are the single source of truth for both the struct members
    // and the size constants: add/remove a material/texture/mesh field in the list
    // and kMin*RecordSize follows automatically (static_asserts pin the current
    // values, see FAssetReader.cpp).
    namespace detail
    {
        template <typename T> struct DiskMinSize { static constexpr uint32_t Value = 4; };          // enums / int32 / uint32 / float
        template <> struct DiskMinSize<bool>       { static constexpr uint32_t Value = 1; };
        template <> struct DiskMinSize<uint64_t>   { static constexpr uint32_t Value = 8; };
        template <> struct DiskMinSize<linalg::vec<float, 2>> { static constexpr uint32_t Value = 8; };  // float2
        template <> struct DiskMinSize<linalg::vec<float, 3>> { static constexpr uint32_t Value = 12; }; // float3
        template <> struct DiskMinSize<linalg::vec<float, 4>> { static constexpr uint32_t Value = 16; }; // float4
        template <> struct DiskMinSize<linalg::mat<float, 4, 4>> { static constexpr uint32_t Value = 64; }; // float4x4
        template <> struct DiskMinSize<eastl::string> { static constexpr uint32_t Value = 4; };    // 4B length prefix
        template <typename T> struct DiskMinSize<eastl::vector<T>> { static constexpr uint32_t Value = 4; }; // 4B count prefix
    }

    // Field-list macros: X(Name, Type, [= Default]) invoked with VTNA_DECLARE_MEMBER
    // (struct member) or VTNA_DISK_SUM (record size). Order MUST match the serialization
    // order in FAssetWriter.cpp / FAssetReader.cpp.
    #define VTNA_DECLARE_MEMBER(Name, Type, ...) Type Name __VA_ARGS__;
    #define VTNA_DISK_SUM(Name, Type, ...) + Assets::detail::DiskMinSize<Type>::Value

    #define VTNA_TEXREF_FIELDS(X)                                            \
        X(RelativePath, eastl::string)                                       \
        X(Usage, ETextureUsage, = ETextureUsage::BaseColor)                  \
        X(bSRGB, bool, = true)                                               \
        X(bHasTransform, bool, = false)                                      \
        X(Offset, float2, = float2(0.0f))                                    \
        X(Scale, float2, = float2(1.0f))                                     \
        X(Rotation, float, = 0.0f)

    #define VTNA_MATERIAL_FIELDS(X)                                          \
        X(Name, eastl::string)                                               \
        X(WorkFlow, EMaterialWorkFlow, = EMaterialWorkFlow::PBRMetallicRoughness) \
        X(Textures, eastl::vector<FAssetTextureRef>)                         \
        X(AlbedoColor, float3, = float3(1.0f))                               \
        X(Metallic, float, = 0.0f)                                           \
        X(Roughness, float, = 0.0f)                                          \
        X(DiffuseColor, float3, = float3(1.0f))                              \
        X(SpecularColor, float3, = float3(0.0f))                             \
        X(Glossiness, float, = 0.0f)                                         \
        X(EmissiveColor, float3, = float3(0.0f))                             \
        X(AlphaCutout, float, = 0.0f)                                        \
        X(bAlphaTest, bool, = false)                                         \
        X(bAlphaBlend, bool, = false)                                        \
        X(bDoubleSided, bool, = false)

    #define VTNA_MESH_FIELDS(X)                                              \
        X(Name, eastl::string)                                               \
        X(MaterialIndex, int32_t, = -1)                                      \
        X(Center, float3, = float3(0.0f))                                    \
        X(Radius, float, = 0.0f)                                             \
        X(IndexCount, uint32_t, = 0)                                         \
        X(VertexCount, uint32_t, = 0)                                        \
        X(IndexStride, uint32_t, = 2)                                        \
        X(IndexData, eastl::vector<uint8_t>)                                 \
        X(bSkeletal, bool, = false)                                          \
        X(bSkeletalAnim, bool, = false)                                      \
        X(Streams, eastl::vector<FStream>)                                   \
        X(MeshletCount, uint32_t, = 0)                                       \
        X(MeshletBounds, eastl::vector<FMeshlet>)                            \
        X(MeshletIndexData, eastl::vector<uint8_t>)                          \
        X(MeshletVertexData, eastl::vector<uint8_t>)

    // [0..48)    FVAssetHeader fixed header (all fixed offsets, no variable length)
    // [48..)     FAssetSectionDesc × SectionCount (24B each)
    // [align16)  section 0: Metadata (source stats/path/references)
    // [align16)  section 1: Model or Texture payload
    
    enum class EAssetType : uint32_t
    {
        Model   = 0,
        Texture = 1,
    };

    enum class EVertexStreamType : uint32_t
    {
        Position    = 0,
        TexCoord    = 1,
        Normal      = 2,
        Tangent     = 3,
        Joints      = 4,
        Weights     = 5,
    };

    enum class ETextureUsage : uint32_t
    {
        BaseColor           = 0,
        MetallicRoughness   = 1,
        Normal              = 2,
        Emissive            = 3,
        AmbientOcclusion    = 4,
        Diffuse             = 5,
        SpecularGlossiness  = 6,
        EnvironmentHDR      = 7,
        Cubemap             = 8,
    };

    enum class ETextureFormat : uint32_t
    {
        Unknown     = 0,
        RGBA32F     = 1,
        RGBA16F     = 2,
        RGBA8UNORM  = 3,
        RGBA8SRGB   = 4,
        RG16F       = 5,
        RG8UNORM    = 6,
        R32F        = 7,
        R16F        = 8,
        R8UNORM     = 9,
        D32F        = 10,
        BC1UNORM    = 11,
        BC1SRGB     = 12,
        BC3UNORM    = 13,
        BC3SRGB     = 14,
        BC4UNORM    = 15,
        BC5UNORM    = 16,
        BC6HU16F    = 17,
        BC7UNORM    = 18,
        BC7SRGB     = 19,
    };

    enum class ETextureType : uint32_t
    {
        Unknown           = 0,
        Texture2D         = 1,
        Texture2DArray    = 2,
        Texture3D         = 3,
        TextureCube       = 4,
        TextureCubeArray  = 5,
    };

    enum class EMaterialWorkFlow : uint32_t
    {
        PBRMetallicRoughness  = 0,
        PBRSpecularGlossiness = 1,
    };

    // Vertex stream (namespace scope so the serialized-size field list below can reference
    // it outside the struct; FMeshCookedData::FStream alias keeps existing call sites).
    struct FStream
    {
        EVertexStreamType Type = EVertexStreamType::Position;
        uint32_t Stride = 0;
        eastl::vector<uint8_t> Data;
    };

    // Mesh Payload
    struct FMeshCookedData
    {
        using FStream = ::Assets::FStream; // compatibility alias (was a nested struct)

        // bSkeletal == false: Meshlet* fields below are static-mesh only
        VTNA_MESH_FIELDS(VTNA_DECLARE_MEMBER)
    };

    // Texture reference; RelativePath stores the .vasset output path (relative to AssetsPath)
    struct FAssetTextureRef
    {
        VTNA_TEXREF_FIELDS(VTNA_DECLARE_MEMBER)
    };

    // Material table entry
    struct FAssetMaterial
    {
        VTNA_MATERIAL_FIELDS(VTNA_DECLARE_MEMBER)
    };

    // Node (written for both static and skeletal; stores model-local TRS, no instance-level m_MtxWorld)
    // bFrontFaceCCW lives here, not on the material: front-face winding is a per-node property
    // (a mirrored node transform flips triangle winding), while FMaterial/PSO is per-material.
    // The rasterizer state is still built from the material at load time (Slice 3 feeds this in).
    struct FAssetNode
    {
        eastl::string Name;
        int32_t Parent          = -1;
        eastl::vector<int32_t> Children;
        float3 Translation      = float3(0.0f);
        float4 Rotation         = float4(0.0f, 0.0f, 0.0f, 1.0f);
        float3 Scale            = float3(1.0f);
        bool bFrontFaceCCW      = false; // true = front face is counter-clockwise (determinant(mtx) > 0)
        eastl::vector<int32_t> Primitives; // M6: primitive indices attached to this node (node->mesh one-to-many; sole mesh reference, old MeshIndex leftover removed)
    };

    struct FAssetAnimationChannel
    {
        int32_t TargetNode = -1;
        uint32_t Mode      = 0; // 0=translation, 1=rotation, 2=scale (matches EAnimationChannelMode values)
        eastl::vector<eastl::pair<float, float4>> KeyFrames;
    };

    struct FAssetAnimation
    {
        eastl::string Name;
        eastl::vector<FAssetAnimationChannel> Channels;
        float TimeDuration = 0.0f;
    };

    struct FAssetSkeleton
    {
        eastl::vector<int32_t> Joints;
        eastl::vector<float4x4> InverseBindMatrices;
    };

    // Model payload (everything from one source model file)
    struct FModelCookedData
    {
        eastl::vector<FMeshCookedData> Meshes;
        eastl::vector<FAssetMaterial> Materials;
        eastl::vector<FAssetNode> Nodes;  // both static and skeletal
        eastl::vector<int32_t> RootNodes; // both static and skeletal
        FAssetSkeleton Skeleton;
        eastl::vector<FAssetAnimation> Animations;
    };

    // ---------------------------------------------------------------------------
    // Record-size lower bounds, derived from the field lists above (FAssetReader.cpp
    // uses them as sanity divisors). Adding/removing a field in the lists updates
    // these automatically; the asserts pin the layout changes from Slice 3 §4.6 M8.
    constexpr uint32_t kMinMeshRecordSize       = 0 VTNA_MESH_FIELDS(VTNA_DISK_SUM);
    constexpr uint32_t kMinMaterialRecordSize   = 0 VTNA_MATERIAL_FIELDS(VTNA_DISK_SUM);
    constexpr uint32_t kMinTextureRefRecordSize = 0 VTNA_TEXREF_FIELDS(VTNA_DISK_SUM);

    static_assert(kMinMeshRecordSize == 62,       "mesh record layout changed (expected 62; Slice 3 4.6 M8)");
    static_assert(kMinMaterialRecordSize == 79,   "material record layout changed (expected 79; Slice 3 4.6 M8, ShadingModel removed)");
    static_assert(kMinTextureRefRecordSize == 30, "texture ref record layout changed (expected 30; Slice 3 4.6 M8)");

    // ETexture payload
    struct FTextureCookedData
    {
        uint32_t Width          = 1;
        uint32_t Height         = 1;
        uint32_t Depth          = 1;
        uint32_t MipLevels      = 1;
        uint32_t ArraySize      = 1;
        ETextureFormat Format   = ETextureFormat::Unknown;
        ETextureType Type       = ETextureType::Texture2D;
        eastl::vector<uint8_t> MipData; // mip0..n row-major, DDS-like layout (as UploadTexture expects)
    };

    // ---- Disk container: fixed header + section table + section data ----
    // Fixed-size header (48B, fixed offsets, no variable-length fields) guarantees: IsUpToDate from the
    // header alone, unknown sections can be skipped, downstream append-only section growth.
    // Variable-length source info is demoted to the Metadata section.

    // Cooker format is pre-1.0: encoded as (major << 16) | minor, printed "0.2".
    // v0.1 = Slice 1 layout; v0.2 = bFrontFaceCCW moved from FAssetMaterial to FAssetNode
    // (per-node winding). Bump minor on layout changes; bump major only when the format
    // stabilizes at 1.0.
    static constexpr uint32_t kAssetCookerMajorVersion = 0;
    static constexpr uint32_t kAssetCookerMinorVersion = 4;
    static constexpr uint32_t kAssetCookerVersion = (kAssetCookerMajorVersion << 16) | kAssetCookerMinorVersion;
    static constexpr uint32_t kAssetHeaderSize = 48;

    enum class EAssetSection : uint32_t
    {
        Metadata    = 0,    // source stat/path/refs (variable-length)
        Model       = 1,    // FModelCookedData payload
        Texture     = 2,    // FTextureCookedData payload
        // Future: MeshletHierarchy = 3
    };

    struct FAssetSectionDesc // 24B
    {
        EAssetSection Type      = EAssetSection::Metadata;
        uint32_t Reserved       = 0;
        uint64_t Offset         = 0; // relative to file start
        uint64_t Size           = 0;
    };

    struct FVAssetHeader
    {
        static constexpr uint32_t MagicConst        = 0x564E4154u;          // "VTNA"
        static constexpr uint32_t CurrentVersion    = 1;
        uint32_t Magic                              = MagicConst;
        uint32_t Version                            = CurrentVersion;
        uint32_t CookerVersion                      = kAssetCookerVersion;
        EAssetType Type                             = EAssetType::Model;
        uint32_t SectionCount                       = 0;
        uint32_t Reserved                           = 0;
        uint64_t PayloadOffset                      = 0;                    // offset of the first section's data
        uint64_t PayloadSize                        = 0;                    // total size of all section data
        uint64_t ContentHash                        = 0;

        // The following variable-length fields are serialized into the Metadata section, not read from the fixed header:
        uint64_t SourceMTime                        = 0;
        uint64_t SourceSize                         = 0;
        eastl::string SourcePath;
        eastl::vector<eastl::string> References; // .vasset output paths of dependencies
    };
}