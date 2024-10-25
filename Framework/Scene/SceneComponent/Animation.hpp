#pragma once

#include "Utilities/Math.hpp"

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Assets
{
    class ModelLoader;
};

namespace Scene
{
    enum class EAnimationChannelMode
    {
        Translation,
        Rotatioin,
        Scale,
    };

    struct FAnimationChannel
    {
        uint32_t TargetNode;
        EAnimationChannelMode Mode;
        eastl::vector<eastl::pair<float, float4>> KeyFrames;
    };

    class SkeletalMesh;

    class Animation
    {
        friend class Assets::ModelLoader;

    public:
        Animation(const eastl::string& name);
        void Update(SkeletalMesh* mesh, float deltaTime);
    
    private:
        void UpdateChannel(SkeletalMesh* mesh, const FAnimationChannel& channel);
    
    private:
        eastl::string mName;
        eastl::vector<FAnimationChannel> mChannels;
        float mTimeDuration = 0.0f;
        float mCurrentAnimTime = 0.0f;
    };
}