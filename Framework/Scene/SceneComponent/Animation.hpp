#pragma once

#include "Utilities/Math.hpp"

#include <string>

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
        std::vector<std::pair<float, float4>> KeyFrames;
    };

    class SkeletonMesh;

    class Animation
    {
        friend class Assets::ModelLoader;

    public:
        Animation(const std::string& name);
        void Update(SkeletonMesh* mesh, float deltaTime);
    
    private:
        void UpdateChannel(SkeletonMesh* mesh, const FAnimationChannel& channel);
    
    private:
        std::string mName;
        std::vector<FAnimationChannel> mChannels;
        float mTimeDuration = 0.0f;
        float mCurrentAnimTime = 0.0f;
    };
}