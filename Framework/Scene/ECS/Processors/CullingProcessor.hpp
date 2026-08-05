#pragma once

#include "Scene/ECS/ProcessorBase.hpp"
#include "Utilities/Math.hpp"

#include <EASTL/span.h>
#include <functional>

namespace Scene::ECS
{
    // Processor that performs frustum culling for visible objects in the scene.
    class FCullingProcessor final : public FProcessorBase
    {
    public:
        using FParallelForFunc = void(*)(uint32_t count, const std::function<void(uint32_t)>& func);

        explicit FCullingProcessor(eastl::span<const float4> frustumPlanes, FParallelForFunc parallelFor = nullptr)
            : m_FrustumPlanes(frustumPlanes), m_ParallelFor(parallelFor) {}

        EProcessingPhase GetProcessingPhase() const override { return EProcessingPhase::Update; }
        const char *GetName() const override { return "CullingProcessor"; }
        void Process(FEntitySubsystem &subsystem, float deltaTime) override;
        void SetFrustumPlanes(eastl::span<const float4> planes) { m_FrustumPlanes = planes; }

    private:
        eastl::span<const float4> m_FrustumPlanes;
        FParallelForFunc m_ParallelFor = nullptr;
    };
} // namespace Scene::ECS