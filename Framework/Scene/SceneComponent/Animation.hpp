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

        void ResetAnimation() { m_CurrentAnimTime = 0.0f; }
    
    private:
        void UpdateChannel(SkeletalMesh* mesh, const FAnimationChannel& channel);
    
    private:
        eastl::string m_Name;
        eastl::vector<FAnimationChannel> m_Channels;
        float m_TimeDuration = 0.0f;
        float m_CurrentAnimTime = 0.0f;
    };
}