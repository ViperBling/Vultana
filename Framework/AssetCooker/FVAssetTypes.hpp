#pragma once

#include "Utilities/Math.hpp"
#include "Common/Meshlet.hlsli"

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/utility.h>

#include <cstdint>

namespace Assets
{
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

    enum class EShadingModel : uint32_t
    {
        DefaultPBR = 0,
        // Unlit / Cloth / Clearcoat 
    };

    // Mesh Payload
    struct FMeshCookedData
    {
        eastl::string Name;
        int32_t MaterialIndex   = -1;
        float3 Center           = float3(0.0f);
        float Radius            = 0.0f;
        uint32_t IndexCount     = 0;
        uint32_t VertexCount    = 0;
        uint32_t IndexStride    = 2;
        eastl::vector<uint8_t> IndexData;
        bool bSkeletal          = false;
        bool bSkeletalAnim      = false;

        struct FStream
        {
            EVertexStreamType Type = EVertexStreamType::Position;
            uint32_t Stride = 0;
            eastl::vector<uint8_t> Data;
        };
        eastl::vector<FStream> Streams;

        // bSkeletal == false
        uint32_t MeshletCount = 0;
        eastl::vector<FMeshlet> MeshletBounds;
        eastl::vector<uint8_t> MeshletIndexData;
        eastl::vector<uint8_t> MeshletVertexData;
    };

    // Texture reference; RelativePath stores the .vasset output path (relative to AssetsPath)
    struct FAssetTextureRef
    {
        eastl::string RelativePath;
        ETextureUsage Usage     = ETextureUsage::BaseColor;
        bool bSRGB              = true;
        bool bHasTransform      = false;
        float2 Offset           = float2(0.0f);
        float2 Scale            = float2(1.0f);
        float Rotation          = 0.0f;
    };

    // Material table entry
    struct FAssetMaterial
    {
        eastl::string Name;
        EMaterialWorkFlow WorkFlow = EMaterialWorkFlow::PBRMetallicRoughness;
        EShadingModel ShadingModel = EShadingModel::DefaultPBR;
        eastl::vector<FAssetTextureRef> Textures;

        // PBR metallic-roughness:
        float3 AlbedoColor         = float3(1.0f);
        float Metallic             = 0.0f;
        float Roughness            = 0.0f;
        // PBR specular-glossiness:
        float3 DiffuseColor        = float3(1.0f);
        float3 SpecularColor       = float3(0.0f);
        float Glossiness           = 0.0f;
        // General:
        float3 EmissiveColor       = float3(0.0f);
        float AlphaCutout          = 0.0f;
        bool bAlphaTest            = false;
        bool bAlphaBlend           = false;
        bool bDoubleSided          = false;
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
    static constexpr uint32_t kAssetCookerMinorVersion = 2;
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