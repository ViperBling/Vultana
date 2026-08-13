#include "Scene/ECS/Processors/CullingProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/Log.hpp"

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

        m_ScratchEntities.clear();
        m_ScratchEntities.reserve(view.size_hint());
        view.each([this](entt::entity entity, const FWorldBoundingSphereComponent&, const FVisibilityComponent&)
                  { m_ScratchEntities.push_back(entity); });
        if (m_ScratchEntities.empty())
            return;

        auto cullFunc = [&](uint32_t i)
        {
            auto entity = m_ScratchEntities[i];
            auto& bounds = view.get<FWorldBoundingSphereComponent>(entity);
            auto& visibility = view.get<FVisibilityComponent>(entity);
            visibility.bFrustumVisible = SphereFrustumCull(m_FrustumPlanes, bounds.Center, bounds.Radius);
        };

        if (m_ParallelFor)
        {
            m_ParallelFor(static_cast<uint32_t>(m_ScratchEntities.size()), cullFunc);
        }
        else
        {
            // Serial fallback keeps the ECS core free of engine include dependencies
            // (Utilities/ParallelFor.hpp pulls in Core/VultanaEngine.hpp).
            // The engine layer always injects the parallel implementation instead.
            static bool bWarnedSerial = false;
            if (!bWarnedSerial)
            {
                bWarnedSerial = true;
                VTNA_LOG_WARN("FCullingProcessor: running serial fallback — no parallel implementation injected");
            }
            for (uint32_t i = 0; i < (uint32_t)m_ScratchEntities.size(); ++i)
                cullFunc(i);
        }
    }
}
