#pragma once

#include "Utilities/Math.hpp"

namespace Scene::ECS
{
    struct FSceneLightSnapshot
    {
        float3 Color     = float3(1.0f);                // matches ILight default
        float3 Direction = float3(0.0f, 1.0f, 0.0f);    // matches ILight default (rotation fix owned by 3.5)
        float Radius     = 0.005f;                      // matches ILight default
        bool bValid      = false;
    };
}