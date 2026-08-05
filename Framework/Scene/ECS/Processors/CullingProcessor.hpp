#pragma once

#include "Scene/ECS/ProcessorBase.hpp"
#include "Utilities/Math.hpp"

#include <functional>

namespace Scene::ECS
{
    // Processor that performs frustum culling for visible objects in the scene.
    class FCullingProcessor final : public FProcessorBase
    {
    public:
        using FParallelForFunc = void(*)(uint32_t count, const std::function<void(uint32_t)>& func);

        explicit FCullingProcessor(const float4 *frustumPlanes, uint32_t planeCount, FParallelForFunc parallelFor = nullptr)
            : m_FrustumPlanes(frustumPlanes), m_PlaneCount(planeCount), m_ParallelFor(parallelFor) {}

        EProcessingPhase GetProcessingPhase() const override { return EProcessingPhase::Update; }
        const char *GetName() const override { return "CullingProcessor"; }
        void Process(FEntitySubsystem &subsystem, float deltaTime) override;
        void SetFrustumPlanes(const float4 *planes, uint32_t count);

    private:
        const float4* m_FrustumPlanes = nullptr;
        uint32_t m_PlaneCount = 0;
        FParallelForFunc m_ParallelFor = nullptr;
    };
} // namespace Scene::ECS