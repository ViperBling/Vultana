#include "Scene/ECS/Processors/CameraSyncProcessor.hpp"
#include "Scene/ECS/EntitySubsystem.hpp"
#include "Scene/ECS/Components.hpp"

namespace Scene::ECS
{
    void FCameraSyncProcessor::Process(FEntitySubsystem& subsystem, float deltaTime)
    {
        if (m_pOutput == nullptr)
            return;
        auto& registry = subsystem.GetRegistry();
        *m_pOutput = FSceneCameraSnapshot{};

        auto camView = registry.view<const FTransformComponent, const FCameraComponent, const FMainCameraTag>();
        if (const entt::entity cam = camView.front(); cam != entt::null)
        {
            const auto& tf = camView.get<const FTransformComponent>(cam);
            const auto& camComp = camView.get<const FCameraComponent>(cam);
            m_pOutput->CameraEntity = cam;
            m_pOutput->Position = tf.Position;
            m_pOutput->Rotation = tf.Rotation;
            m_pOutput->Fov = camComp.Fov;
            m_pOutput->bValid = true;
        }
    }
}