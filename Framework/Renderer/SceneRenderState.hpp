#pragma once

#include "Utilities/Math.hpp"
#include "Common/GlobalConstants.hlsli"

namespace Renderer
{
    struct FSceneRenderState
    {
        FCameraConstants CameraConstants;
        float3 LightColor         = float3(1.0f);
        float3 LightDirection     = float3(0.0f, -1.0f, 0.0f);
        float LightRadius         = 0.0f;
        bool bCameraValid         = false;
        bool bLightValid          = false;
    };
}