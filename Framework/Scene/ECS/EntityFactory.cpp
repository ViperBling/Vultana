#include "Scene/ECS/EntityFactory.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/Log.hpp"
#include "EntityFactory.hpp"

namespace Scene::ECS
{
    FEntityFactory::FEntityFactory(FEntitySubsystem& subsystem, IAssetRegistry& registry)
        : m_Subsystem(subsystem), m_Registry(registry) {}
    
    entt::entity FEntityFactory::CreateModelInstance(FModelAssetHandle assetHandle, const eastl::string& filePath, entt::entity parent)
    {
        auto& registry = m_Subsystem.GetRegistry();
        entt::entity rootEntity = m_Subsystem.CreateEntity();
        registry.emplace<FNameComponent>(rootEntity);
        auto* nameComp = registry.try_get<FNameComponent>(rootEntity);
        if (nameComp)
        {
            nameComp->SetName(filePath.c_str());
        }
        registry.emplace<FTransformComponent>(rootEntity);
        registry.emplace<FModelInstanceComponent>(rootEntity, assetHandle, filePath);
        if (parent != entt::null)
        {
            m_Subsystem.SetParent(rootEntity, parent);
        }
        VTNA_LOG_INFO("EntityFactory: Created model instance for {}", filePath);
        return rootEntity;
    }

    entt::entity FEntityFactory::CreateEmpty(const char* name, entt::entity parent)
    {
        auto& registry = m_Subsystem.GetRegistry();
        entt::entity entity = m_Subsystem.CreateEntity();
        registry.emplace<FNameComponent>(entity);
        auto* nameComp = registry.try_get<FNameComponent>(entity);
        if (nameComp)
        {
            nameComp->SetName(name);
        }
        registry.emplace<FTransformComponent>(entity);
        if (parent != entt::null)
        {
            m_Subsystem.SetParent(entity, parent);
        }
        VTNA_LOG_INFO("EntityFactory: Created empty entity {}", name);
        return entity;
    }

    void FEntityFactory::DestroyModelInstance(entt::entity rootEntity)
    {
        auto children = m_Subsystem.GetChildren(rootEntity);
        for (entt::entity child : children)
        {
            DestroyModelInstance(child);
        }
        m_Subsystem.DestroyEntityDeferred(rootEntity);
    }

} // namespace Scene::ECS
