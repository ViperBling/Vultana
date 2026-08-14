#include "Scene/ECS/Processors/AnimationProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include "Scene/ECS/AssetRegistry.hpp"
#include "Utilities/Math.hpp"

#include <cmath>
#include <algorithm>
#include "AnimationProcessor.hpp"

namespace Scene::ECS
{
    void EvalChannel(const FAnimationClipChannel& channel, float animTime, FTransformComponent& outTF)
    {
        if (channel.KeyFrames.empty())
            return;

        const auto& frames = channel.KeyFrames;
        float t = 0.0f;
        const eastl::pair<float, float4>* lower = &frames[0];
        const eastl::pair<float, float4>* upper = lower;
        for (size_t f = 0; f + 1 < frames.size(); ++f)
        {
            if (frames[f].first <= animTime && frames[f + 1].first >= animTime)
            {
                lower = &frames[f];
                upper = &frames[f + 1];
                t = (animTime - lower->first) / eastl::max_alt(upper->first - lower->first, 1e-8f);
                break;
            }
        }

        switch (channel.Mode)
        {
        case EAnimChannelMode::Translation:
        {
            const float3 v = lerp(lower->second, upper->second, t).xyz();
            outTF.Position = float3(v.x, v.y, -v.z);
            break;
        }
        case EAnimChannelMode::Rotation:
        {
            const float4 q = RotationSlerp(lower->second, upper->second, t); // shortest-path + near-parallel blend
            outTF.Rotation = float4(q.x, q.y, -q.z, -q.w);
            break;
        }
        case EAnimChannelMode::Scale:
            outTF.Scale = lerp(lower->second, upper->second, t).xyz();
            break;
        default:
            break;
        }
    }

    void FAnimationProcessor::Process(FEntitySubsystem& subsystem, float deltaTime)
    {
        if (m_Registry == nullptr)
            return;
        auto& registry = subsystem.GetRegistry();

        registry.view<FAnimationComponent, FModelNodeTableComponent, FModelInstanceComponent>().each(
        [&](entt::entity root, FAnimationComponent& anim, const FModelNodeTableComponent& nodeTable, const FModelInstanceComponent& modelComp)
        {
            if (!modelComp.ModelHandle.IsValid())
                return;

            const FAnimationClip* clip = m_Registry->FindAnimation(modelComp.ModelHandle, anim.ClipIndex);

            if (anim.bPlaying && clip != nullptr)
            {
                const float duration = eastl::max_alt(clip->TimeDuration, 1e-6f);
                anim.CurrentTime = fmodf(anim.CurrentTime + deltaTime, duration);
            }
            if (clip == nullptr)
                return;

            for (const FAnimationClipChannel& channel : clip->Channels)
            {
                if (channel.TargetNode < 0 || static_cast<uint32_t>(channel.TargetNode) >= nodeTable.NodeEntities.size())
                    continue;
                const entt::entity nodeEntity = nodeTable.NodeEntities[static_cast<uint32_t>(channel.TargetNode)];
                if (nodeEntity == entt::null || !registry.valid(nodeEntity))
                    continue;
                if (auto* tf = registry.try_get<FTransformComponent>(nodeEntity))
                    EvalChannel(channel, anim.CurrentTime, *tf);
            }
        });
    }
} // namespace Scene::ECS
