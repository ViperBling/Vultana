#pragma once

#include "Utilities/Math.hpp"
#include <cstdint>
#include <cstring>

#include <entt/entt.hpp>

namespace Scene::ECS
{
    // ===== Pick ID ===== //
    struct FPickIDFragment
    {
        uint32_t PickID = 0;
    };

    // ===== Name ===== //
    struct FNameFragment
    {
        char Name[64] = {};

        void SetName(const char* inName)
        {
            strncpy_s(Name, inName, _TRUNCATE);
            Name[63] = '\0';
        }
    };

    // ===== Transform ===== //
    struct FTransformFragment
    {
        float3 Position = float3(0.0f);
        float4 Rotation = float4(0.0f, 0.0f, 0.0f, 1.0f);
        float3 Scale    = float3(1.0f);
    };

    // ===== Parent ===== //
    struct FParentFragment
    {
        entt::entity Parent = entt::null;
    };

    // ===== World Transform ===== //
    struct FWorldTransformFragment
    {
        float4x4 MtxWorld             = float4x4(1.0f);
        float4x4 MtxWorldInvTranspose = float4x4(1.0f);
    };

    // ===== Bounding Sphere ===== //
    struct FBoundingSphereFragment
    {
        float3 Center = float3(0.0f);
        float  Radius = 0.0f;
    };

    // ===== World Bounding Sphere ===== //
    struct FWorldBoundingSphereFragment
    {
        float3 Center = float3(0.0f);
        float  Radius = 0.0f;
    };

    // ===== Bounding Box ===== //
    struct FBoundingBoxFragment
    {
        float3 Min = float3(0.0f);
        float3 Max = float3(0.0f);
    };

    // ===== World Bounding Box ===== //
    struct FWorldBoundingBoxFragment
    {
        float3 Min = float3(0.0f);
        float3 Max = float3(0.0f);
    };

    // ===== Visibility ===== //
    struct FVisibilityFragment
    {
        bool bFrustumVisible = false;
    };

    // ===== GUID ===== //
    struct FSceneGUIDFragment
    {
        uint64_t GUID = 0;
    };
}