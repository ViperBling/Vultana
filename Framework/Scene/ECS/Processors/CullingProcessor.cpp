#include "Scene/ECS/Processors/CullingProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/ParallelFor.hpp"
#include "CullingProcessor.hpp"

namespace Scene::ECS
{
    inline bool SphereFrustumCull(eastl::span<const float4> planes, const float3& center, float radius)
    {
        for (auto& p : planes)
        {
            if (p.x * center.x + p.y * center.y + p.z * center.z + p.w <= -radius)
                return false;
        }
        return true;
    }

    void FCullingProcessor::Process(FEntitySubsystem& subsystem, float deltaTime)
    {
        if (m_FrustumPlanes.empty())
            return;
        auto& registry = subsystem.GetRegistry();
        auto view = registry.view<FWorldBoundingSphereComponent, FVisibilityComponent>();
        eastl::vector<entt::entity> entities;
        entities.reserve(view.size_hint());
        view.each([&entities](entt::entity entity, const FWorldBoundingSphereComponent&, const FVisibilityComponent&)
        {
            entities.push_back(entity);
        });
        if (entities.empty())
            return;

        auto cullFunc = [&](uint32_t i)
        {
            auto entity = entities[i];
            auto& bounds = view.get<FWorldBoundingSphereComponent>(entity);
            auto& visibility = view.get<FVisibilityComponent>(entity);
            visibility.bFrustumVisible = SphereFrustumCull(m_FrustumPlanes, bounds.Center, bounds.Radius);
        };

        if (m_ParallelFor)
            m_ParallelFor(static_cast<uint32_t>(entities.size()), cullFunc);
        else
            Utilities::ParallelFor(static_cast<uint32_t>(entities.size()), cullFunc);
    }
}
