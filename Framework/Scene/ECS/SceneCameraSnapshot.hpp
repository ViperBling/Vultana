#pragma once

#include "Utilities/Math.hpp"
#include <entt/entt.hpp>

namespace Scene::ECS
{
    struct FSceneCameraSnapshot
    {
        entt::entity CameraEntity = entt::null;
        float3 Position           = float3(0.0f);
        float4 Rotation           = float4(0.0f, 0.0f, 0.0f, 1.0f); // quaternion (x,y,z,w)
        float Fov                 = 45.0f;
        bool bValid               = false;
    };
} // namespace Scene::ECS