#pragma once

#include "Utilities/Math.hpp"

#include <EASTL/string.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>

namespace Scene::ECS
{
    enum class EAnimChannelMode
    {
        Translation = 0,
        Rotation = 1,
        Scale = 2,
    };

    struct FAnimationClipChannel
    {
        int32_t TargetNode = -1;
        EAnimChannelMode Mode = EAnimChannelMode::Translation;
        eastl::vector<eastl::pair<float, float4>> KeyFrames;
    };

    struct FAnimationClip
    {
        eastl::string Name;
        eastl::vector<FAnimationClipChannel> Channels;
        float TimeDuration = 0.0f;
    };

    struct FSkeletonInfo
    {
        eastl::vector<int32_t> JointNodeIndices;
        eastl::vector<float4x4> InverseBindMatrices;
    };
}