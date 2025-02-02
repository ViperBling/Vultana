#include "Animation.hpp"
#include "SkeletalMesh.hpp"

namespace Scene
{
    Animation::Animation(const eastl::string &name)
    {
        mName = name;
    }

    void Animation::Update(SkeletalMesh *mesh, float deltaTime)
    {
        mCurrentAnimTime += deltaTime;
        if (mCurrentAnimTime > mTimeDuration)
        {
            mCurrentAnimTime = mCurrentAnimTime - mTimeDuration;
        }
        for (size_t i = 0; i < mChannels.size(); i++)
        {
            const FAnimationChannel& channel = mChannels[i];
            UpdateChannel(mesh, channel);
        }
    }

    void Animation::UpdateChannel(SkeletalMesh *mesh, const FAnimationChannel &channel)
    {
        eastl::pair<float, float4> lowerFrame;
        eastl::pair<float, float4> upperFrame;

        bool found = false;
        for (size_t frame = 0; frame < channel.KeyFrames.size() - 1; ++frame)
        {
            if (channel.KeyFrames[frame].first <= mCurrentAnimTime &&
                channel.KeyFrames[frame + 1].first >= mCurrentAnimTime)
            {
                lowerFrame = channel.KeyFrames[frame];
                upperFrame = channel.KeyFrames[frame + 1];
                found = true;
                break;
            }
        }

        float interpolationValue;
        if (found)
        {
            interpolationValue = (mCurrentAnimTime - lowerFrame.first) / (upperFrame.first - lowerFrame.first);
        }
        else
        {
            lowerFrame = upperFrame = channel.KeyFrames[0];
            interpolationValue = 0.0f;
        }

        FSkeletalMeshNode* node = mesh->GetNode(channel.TargetNode);

        switch (channel.Mode)
        {
        case EAnimationChannelMode::Translation:
        {
            float3 lowerValue = float3(lowerFrame.second.x, lowerFrame.second.y, -lowerFrame.second.z);
            float3 upperValue = float3(upperFrame.second.x, upperFrame.second.y, -upperFrame.second.z);

            node->Translation = lerp(lowerValue, upperValue, interpolationValue);
            break;
        }
        case EAnimationChannelMode::Rotatioin:
        {
            float4 lowerValue = float4(lowerFrame.second.x, lowerFrame.second.y, -lowerFrame.second.z, -lowerFrame.second.w);
            float4 upperValue = float4(upperFrame.second.x, upperFrame.second.y, -upperFrame.second.z, -upperFrame.second.w);

            node->Rotation = RotationSlerp(lowerValue, upperValue, interpolationValue);
            break;
        }
        case EAnimationChannelMode::Scale:
        {
            node->Scale = lerp(lowerFrame.second.xyz(), upperFrame.second.xyz(), interpolationValue);
            break;
        }
        default:
            break;
        }
    }
}