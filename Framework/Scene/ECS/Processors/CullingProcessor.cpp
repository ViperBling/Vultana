#include "Scene/ECS/Processors/CullingProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/ParallelFor.hpp"
#include "CullingProcessor.hpp"

namespace Scene::ECS
{
    inline bool SphereFrustumCull(const float4* planes, uint32_t planeCount, const float3& center, float radius)
    {
        for (uint32_t i = 0; i < planeCount; ++i)
        {
            const float4& p = planes[i];
            if (p.x * center.x + p.y * center.y + p.z * center.z + p.w <= -radius)
                return false;
        }
        return true;
    }

    void FCullingProcessor::Process(FEntitySubsystem& subsystem, float deltaTime)
    {
        if (!m_FrustumPlanes || m_PlaneCount == 0)
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
            visibility.bFrustumVisible = SphereFrustumCull(m_FrustumPlanes, m_PlaneCount, bounds.Center, bounds.Radius);
        };

        if (m_ParallelFor)
            m_ParallelFor(static_cast<uint32_t>(entities.size()), cullFunc);
        else
            Utilities::ParallelFor(static_cast<uint32_t>(entities.size()), cullFunc);
    }

    void FCullingProcessor::SetFrustumPlanes(const float4 *planes, uint32_t count)
    {
        m_FrustumPlanes = planes;
        m_PlaneCount = count;
    }
}
