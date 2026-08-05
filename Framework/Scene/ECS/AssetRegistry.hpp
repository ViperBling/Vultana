#pragma once

#include "Scene/ECS/Components.hpp"
#include "Utilities/Math.hpp"

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <cstdint>

namespace Scene::ECS
{
    // Mesh GPU data
    struct FMeshGPUData
    {
        uint32_t PositionBufferAddress      = 0;
        uint32_t TexCoordBufferAddress      = 0;
        uint32_t NormalBufferAddress        = 0;
        uint32_t TangentBufferAddress       = 0;
        uint32_t MeshletBufferAddress       = 0;
        uint32_t MeshletVertexBufferAddress = 0;
        uint32_t MeshletIndexBufferAddress  = 0;
        uint32_t MeshletCount               = 0;
        uint32_t IndexBufferAddress         = 0;
        uint32_t IndexStride                = 2;
        uint32_t IndexCount                 = 0;
        float3 Center = float3(0.0f);
        float Radius = 0.0f;
        bool IsValid() const { return MeshletCount > 0; }
    };

    // Material GPU data
    struct FMaterialGPUData
    {
        uint32_t Data[80] = {};
    };

    struct FModelNodeInfo
    {
        eastl::string Name;
        uint32_t NodeIndex      = 0;
        float3   Translation    = float3(0.0f);
        float4   Rotation       = float4(0.0f, 0.0f, 0.0f, 1.0f);
        float3   Scale          = float3(1.0f);
        int32_t  ParentIndex    = -1;
        uint32_t MeshIndex      = UINT32_MAX;
    };

    struct FModelAssetInfo
    {
        eastl::string ModelPath;
        eastl::vector<FModelNodeInfo> Nodes;
        uint32_t PrimitiveCount = 0;
    };

    class IAssetRegistry
    {
    public:
        virtual ~IAssetRegistry() = default;
        virtual FModelAssetHandle ImportModel(const eastl::string& assetFile) = 0;
        virtual FMeshResourceHandle AcquireMesh(FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx) = 0;
        virtual FMaterialResourceHandle AcquireMaterial(FModelAssetHandle asset, uint32_t nodeIdx, uint32_t primitiveIdx) = 0;
        virtual void Release(FMeshResourceHandle handle) = 0;
        virtual void Release(FMaterialResourceHandle handle) = 0;
        virtual FMeshGPUData GetMeshData(FMeshResourceHandle handle) const = 0;
        virtual FMaterialGPUData GetMaterialData(FMaterialResourceHandle handle) const = 0;
        virtual FModelAssetInfo GetModelInfo(FModelAssetHandle handle) const = 0;
        virtual void FrameFencePassed(uint64_t frameID) = 0;
    };
}