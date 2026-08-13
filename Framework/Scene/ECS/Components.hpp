#pragma once

#include "Utilities/Math.hpp"
#include <cstdint>

#include <EASTL/string.h>
#include <entt/entt.hpp>

namespace Scene::ECS
{
    // ===== Pick ID ===== //
    struct FPickIDComponent
    {
        uint32_t PickID = 0;
    };

    // ===== Name ===== //
    struct FNameComponent
    {
        char Name[64] = {};

        void SetName(const char* inName)
        {
            strncpy_s(Name, inName, _TRUNCATE);
            Name[63] = '\0';
        }
    };

    // ===== Transform ===== //
    struct FTransformComponent
    {
        float3 Position = float3(0.0f);
        float4 Rotation = float4(0.0f, 0.0f, 0.0f, 1.0f);
        float3 Scale    = float3(1.0f);
    };

    // ===== Parent ===== //
    struct FParentComponent
    {
        entt::entity Parent = entt::null;
    };

    // ===== World Transform ===== //
    struct FWorldTransformComponent
    {
        float4x4 MtxWorld             = float4x4({identity});
        float4x4 MtxWorldInvTranspose = float4x4({identity});
    };

    // ===== Bounding Sphere ===== //
    struct FBoundingSphereComponent
    {
        float3 Center = float3(0.0f);
        float  Radius = 0.0f;
    };

    // ===== World Bounding Sphere ===== //
    struct FWorldBoundingSphereComponent
    {
        float3 Center = float3(0.0f);
        float  Radius = 0.0f;
    };

    // ===== Bounding Box ===== //
    struct FBoundingBoxComponent
    {
        float3 Min = float3(0.0f);
        float3 Max = float3(0.0f);
    };

    // ===== World Bounding Box ===== //
    struct FWorldBoundingBoxComponent
    {
        float3 Min = float3(0.0f);
        float3 Max = float3(0.0f);
    };

    // ===== Visibility ===== //
    struct FVisibilityComponent
    {
        bool bFrustumVisible = false;
    };

    // ===== GUID ===== //
    struct FSceneGUIDComponent
    {
        uint64_t GUID = 0;
    };

    // ===== Resource Handle ===== //
    struct FMeshResourceHandle
    {
        uint32_t Index = UINT32_MAX;
        uint32_t Generation = 0;
        bool IsValid() const { return Index != UINT32_MAX; }
        bool operator==(const FMeshResourceHandle& o) const { return Index == o.Index && Generation == o.Generation; }
        bool operator!=(const FMeshResourceHandle& o) const { return !(*this == o); }
    };

    struct FMaterialResourceHandle
    {
        uint32_t Index = UINT32_MAX;
        uint32_t Generation = 0;
        bool IsValid() const { return Index != UINT32_MAX; }
        bool operator==(const FMaterialResourceHandle& o) const { return Index == o.Index && Generation == o.Generation; }
        bool operator!=(const FMaterialResourceHandle& o) const { return !(*this == o); }
    };

    struct FModelAssetHandle
    {
        uint32_t Index = UINT32_MAX;
        uint32_t Generation = 0;
        bool IsValid() const { return Index != UINT32_MAX; }
        bool operator==(const FModelAssetHandle& o) const { return Index == o.Index && Generation == o.Generation; }
        bool operator!=(const FModelAssetHandle& o) const { return !(*this == o); }
    };

    struct FMeshRendererComponent
    {
        FMeshResourceHandle MeshHandle;
        FMaterialResourceHandle MaterialHandle;
        bool bSkinned = false;
    };

    struct FModelInstanceComponent
    {
        FModelAssetHandle ModelHandle;
        eastl::string ModelPath;
    };
}