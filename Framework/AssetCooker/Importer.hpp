#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "Utilities/Math.hpp"

#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <cstdint>

namespace Assets
{
    // Nodes and vertex streams are pure data with no renderer dependency, so the disk-facing
    // types are reused directly (same rationale as FAssetTextureRef / FAssetAnimation).
    // This avoids a full duplicate type + a needless copy layer.
    using FImportVertexStream = FMeshCookedData::FStream;
    using FImportNode = FAssetNode;

    struct FImportPrimitive
    {
        eastl::string Name;
        int32_t MaterialIndex   = -1;                   // Index into FImportResult::Materials
        uint32_t IndexStride    = 2;
        uint32_t IndexCount     = 0;
        uint32_t VertexCount    = 0;
        eastl::vector<uint8_t> IndexData;
        eastl::vector<FImportVertexStream> Streams;
        bool bSkeletal          = false;                // node->skin != nullptr
        bool bSkeletalAnim      = false;                // Propagated to FMeshCookedData.bSkeletalAnim
        float3 Center           = float3(0.0f);
        float Radius            = 0.0f;
    };

    struct FImportMaterial
    {
        eastl::string Name;
        EMaterialWorkFlow WorkFlow = EMaterialWorkFlow::PBRMetallicRoughness;
        eastl::vector<FAssetTextureRef> Textures;

        float3 AlbedoColor      = float3(1.0f);
        float Metallic          = 0.0f;
        float Roughness         = 0.0f;
        float3 DiffuseColor     = float3(1.0f);
        float3 SpecularColor    = float3(0.0f);
        float Glossiness        = 0.0f;
        float3 EmissiveColor    = float3(0.0f);
        float AlphaCutout       = 0.0f;
        bool bAlphaTest         = false;
        bool bAlphaBlend        = false;
        bool bDoubleSided       = false;
    };

    // Complete import result of one source file (in-memory only, never written to disk)
    struct FImportResult
    {
        eastl::vector<FImportPrimitive> Primitives;
        eastl::vector<FImportNode> Nodes;                   // == FAssetNode
        eastl::vector<int32_t> RootNodes;
        eastl::vector<FImportMaterial> Materials;
        eastl::vector<int32_t> Joints;                      // Joint node indices
        eastl::vector<float4x4> InverseBindMatrices;
        eastl::vector<FAssetAnimation> Animations;          // Reuses the disk-facing FAssetAnimation
    };

    class IImporter
    {
    public:
        virtual ~IImporter() = default;
        // Import a source file into FImportResult. Returns false on failure.
        virtual bool Import(const eastl::string& srcPath, FImportResult& out) = 0;
    };
}