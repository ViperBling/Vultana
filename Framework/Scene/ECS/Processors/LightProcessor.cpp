#include "Scene/ECS/Processors/LightProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"

namespace Scene::ECS
{
    void FLightProcessor::Process(FEntitySubsystem& subsystem, float deltaTime)
    {
        if (m_pOutput == nullptr)
            return;
        auto& registry = subsystem.GetRegistry();
        *m_pOutput = FSceneLightSnapshot{};

        auto lightView = registry.view<const FLightComponent, const FMainLightTag>();
        if (const entt::entity light = lightView.front(); light != entt::null)
        {
            const auto& lightComp = lightView.get<const FLightComponent>(light);
            m_pOutput->Color = lightComp.Color;
            m_pOutput->Direction = lightComp.Direction;
            m_pOutput->Radius = lightComp.Radius;
            m_pOutput->bValid = true;
        }
    }
}