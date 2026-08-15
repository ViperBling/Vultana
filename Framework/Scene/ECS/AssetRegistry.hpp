#pragma once

#include "Scene/ECS/AnimationTypes.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/Math.hpp"

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <cstdint>

namespace Scene::ECS
{
    static constexpr uint32_t INVALID_ADDRESS = 0xFFFFFFFFu;

    // Mesh GPU data
    struct FMeshGPUData
    {
        uint32_t PositionBufferAddress      = 0;
        uint32_t TexCoordBufferAddress      = 0;
        uint32_t NormalBufferAddress        = 0;
        uint32_t TangentBufferAddress       = 0;
        bool     bSkinned                   = false;
        uint32_t JointIDBufferAddress       = INVALID_ADDRESS;
        uint32_t JointWeightBufferAddress   = INVALID_ADDRESS;
        bool     bHasNormalStream           = false;
        bool     bHasTangentStream          = false;
        uint32_t AnimPositionBufferAddress  = INVALID_ADDRESS;
        uint32_t AnimNormalBufferAddress    = INVALID_ADDRESS;
        uint32_t AnimTangentBufferAddress   = INVALID_ADDRESS;
        uint32_t MeshletBufferAddress       = 0;
        uint32_t MeshletVertexBufferAddress = 0;
        uint32_t MeshletIndexBufferAddress  = 0;
        uint32_t MeshletCount               = 0;
        uint32_t IndexBufferAddress         = 0;
        uint32_t IndexStride                = 2;
        uint32_t IndexCount                 = 0;
        uint32_t VertexCount                = 0;
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
        eastl::vector<uint32_t> Primitives;
        bool bFrontFaceCCW = false;
    };

    struct FModelAssetInfo
    {
        eastl::string ModelPath;
        eastl::vector<FModelNodeInfo> Nodes;
        uint32_t PrimitiveCount = 0;
        uint32_t AnimCount = 0;
    };

    enum class EAssetState : uint32_t
    {
        Pending = 0,
        Loading = 1,
        Ready = 2,
        Failed = 3,
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

        virtual const FMeshGPUData* FindMeshData(FMeshResourceHandle handle) const = 0;
        virtual const FMaterialGPUData* FindMaterialData(FMaterialResourceHandle handle) const = 0;

        FMeshGPUData GetMeshData(FMeshResourceHandle handle) const
        {
            const FMeshGPUData* data = FindMeshData(handle);
            return data ? *data : FMeshGPUData{};
        }
        FMaterialGPUData GetMaterialData(FMaterialResourceHandle handle) const
        {
            const FMaterialGPUData* data = FindMaterialData(handle);
            return data ? *data : FMaterialGPUData{};
        }

        virtual FModelAssetInfo GetModelInfo(FModelAssetHandle handle) const = 0;
        virtual EAssetState GetState(FModelAssetHandle handle) const = 0;
        bool IsReady(FModelAssetHandle handle) const { return GetState(handle) == EAssetState::Ready; }
        virtual void FrameFencePassed(uint64_t frameID) = 0;

        virtual const FAnimationClip* FindAnimation(FModelAssetHandle asset, uint32_t clipIndex) const = 0;
        virtual const FSkeletonInfo* GetSkeletonInfo(FModelAssetHandle asset) const = 0;

        virtual uint32_t GetMaterialConstantAddress(FMaterialResourceHandle handle) const = 0;
    };
}