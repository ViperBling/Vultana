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

    void FEntityFactory::CreateModelHierarchy(FModelAssetHandle assetHandle, const FModelAssetInfo& info, entt::entity rootEntity, const float3& position, const quaternion& rotation, const float3& scale)
    {
        auto& registry = m_Subsystem.GetRegistry();

        // Root: instance transform (CreateModelInstance already emplaced FTransformComponent).
        auto* rootTF = registry.try_get<FTransformComponent>(rootEntity);
        if (rootTF)
        {
            rootTF->Position = position;
            rootTF->Rotation = float4(rotation);
            rootTF->Scale = scale;
        }

        // Node entities (TRS + parent), attached to the root first.
        eastl::vector<entt::entity> nodeEntities(info.Nodes.size(), entt::entity(entt::null));
        for (uint32_t n = 0; n < (uint32_t)info.Nodes.size(); ++n)
        {
            const FModelNodeInfo& node = info.Nodes[n];
            entt::entity nodeEntity = CreateEmpty(node.Name.empty() ? "Node" : node.Name.c_str(), rootEntity);
            auto* tf = registry.try_get<FTransformComponent>(nodeEntity);
            if (tf)
            {
                tf->Position = node.Translation;
                tf->Rotation = node.Rotation;
                tf->Scale = node.Scale;
            }
            nodeEntities[n] = nodeEntity;
        }

        // Reparent to the real parent (SetParent handles moving between parents).
        for (uint32_t n = 0; n < (uint32_t)info.Nodes.size(); ++n)
        {
            const FModelNodeInfo& node = info.Nodes[n];
            if (node.ParentIndex >= 0 && (uint32_t)node.ParentIndex < nodeEntities.size())
            {
                m_Subsystem.SetParent(nodeEntities[n], nodeEntities[(uint32_t)node.ParentIndex]);
            }
        }

        // Mesh entities: one per primitive, child of its node entity.
        for (uint32_t n = 0; n < (uint32_t)info.Nodes.size(); ++n)
        {
            const FModelNodeInfo& node = info.Nodes[n];
            if (nodeEntities[n] == entt::null) continue;
            for (uint32_t primIdx : node.Primitives)
            {
                Scene::ECS::FMeshResourceHandle meshHandle = m_Registry.AcquireMesh(assetHandle, n, primIdx);
                Scene::ECS::FMaterialResourceHandle matHandle = m_Registry.AcquireMaterial(assetHandle, n, primIdx);
                if (!meshHandle.IsValid()) continue;

                const Scene::ECS::FMeshGPUData meshData = m_Registry.GetMeshData(meshHandle);
                entt::entity meshEntity = CreateEmpty("Mesh", nodeEntities[n]);
                registry.emplace<FMeshRendererComponent>(meshEntity, meshHandle, matHandle, meshData.MeshletCount, false);
                registry.emplace<FBoundingSphereComponent>(meshEntity, meshData.Center, meshData.Radius);
                registry.emplace<FVisibilityComponent>(meshEntity);
            }
        }
    }

} // namespace Scene::ECS
