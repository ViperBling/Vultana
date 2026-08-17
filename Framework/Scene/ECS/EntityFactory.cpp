#include "Scene/ECS/EntityFactory.hpp"
#include "Scene/ECS/Components.hpp"
#include "Utilities/Log.hpp"

#include <EASTL/fixed_vector.h>

namespace Scene::ECS
{
    FEntityFactory::FEntityFactory(FEntitySubsystem& subsystem, IAssetRegistry& registry)
        : m_Subsystem(subsystem), m_Registry(registry) {}

    FEntityFactory::~FEntityFactory()
    {
        DestroyAllModelInstances();
    }

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
        registry.emplace<FWorldTransformComponent>(rootEntity);
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
        registry.emplace<FWorldTransformComponent>(entity);
        if (parent != entt::null)
        {
            m_Subsystem.SetParent(entity, parent);
        }
        VTNA_LOG_INFO("EntityFactory: Created empty entity {}", name);
        return entity;
    }

    void FEntityFactory::DestroyModelInstance(entt::entity rootEntity)
    {
        auto& registry = m_Subsystem.GetRegistry();

        if (auto* meshComp = registry.try_get<FMeshRendererComponent>(rootEntity))
        {
            m_Registry.Release(meshComp->MeshHandle);
            m_Registry.Release(meshComp->MaterialHandle);
        }

        eastl::fixed_vector<entt::entity, 16, true> children;
        if (const eastl::vector<entt::entity>* pChildren = m_Subsystem.FindChildren(rootEntity))
            children.assign(pChildren->begin(), pChildren->end());

        for (entt::entity child : children)
        {
            DestroyModelInstance(child);
        }
        m_Subsystem.DestroyEntityDeferred(rootEntity);
    }

    void FEntityFactory::DestroyAllModelInstances()
    {
        auto& registry = m_Subsystem.GetRegistry();

        eastl::vector<entt::entity> roots;
        for (entt::entity root : registry.view<FModelInstanceComponent>())
            roots.push_back(root);

        for (entt::entity root : roots)
        {
            bool bNested = false;
            entt::entity cur = root;
            while (const FParentComponent* parent = registry.try_get<FParentComponent>(cur))
            {
                if (parent->Parent == entt::null)
                    break;
                cur = parent->Parent;
                if (registry.all_of<FModelInstanceComponent>(cur))
                {
                    bNested = true;
                    break;
                }
            }
            if (!bNested)
                DestroyModelInstance(root);
        }
        m_Subsystem.FlushDeferred();
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
                if (!meshHandle.IsValid() || !matHandle.IsValid())
                {
                    if (meshHandle.IsValid()) m_Registry.Release(meshHandle);
                    if (matHandle.IsValid()) m_Registry.Release(matHandle);
                    VTNA_LOG_WARN("[EntityFactory] node {} primitive {}: mesh/material acquire failed; mesh entity skipped", n, primIdx);
                    continue;
                }

                const Scene::ECS::FMeshGPUData meshData = m_Registry.GetMeshData(meshHandle);
                entt::entity meshEntity = CreateEmpty("Mesh", nodeEntities[n]);
                SetMeshRenderer(registry, meshEntity, meshHandle, matHandle, meshData, assetHandle, static_cast<int32_t>(n), static_cast<int32_t>(primIdx), meshData.bSkinned ? rootEntity : entt::null);
            }
        }
        if (!nodeEntities.empty())
            registry.emplace<FModelNodeTableComponent>(rootEntity, eastl::move(nodeEntities));
        // Animation component only for models that own clips (plays clip 0).
        // It starts paused; the inspector toggles FAnimationComponent::bPlaying.
        if (info.AnimCount > 0)
            registry.emplace<FAnimationComponent>(rootEntity);
    }

    void FEntityFactory::SetMeshRenderer(entt::registry& registry, entt::entity entity, FMeshResourceHandle meshHandle, FMaterialResourceHandle materialHandle, const FMeshGPUData& meshData, FModelAssetHandle asset, int32_t nodeIndex, int32_t primitiveIndex, entt::entity skinnedRoot)
    {
        FModelAssetHandle sourceAsset = asset;
        int32_t sourceNodeIndex = nodeIndex;
        int32_t sourcePrimitiveIndex = primitiveIndex;
        if (const auto* previous = registry.try_get<FMeshRendererComponent>(entity); previous != nullptr && previous->SourceAsset.IsValid() && previous->SourceNodeIndex >= 0 && previous->SourcePrimitiveIndex >= 0)
        {
            sourceAsset = previous->SourceAsset;
            sourceNodeIndex = previous->SourceNodeIndex;
            sourcePrimitiveIndex = previous->SourcePrimitiveIndex;
        }

        registry.emplace_or_replace<FMeshRendererComponent>(entity, meshHandle, materialHandle, meshData.bSkinned, asset, nodeIndex, primitiveIndex, sourceAsset, sourceNodeIndex, sourcePrimitiveIndex);
        registry.emplace_or_replace<FBoundingSphereComponent>(entity, meshData.Center, meshData.Radius);
        registry.emplace_or_replace<FWorldBoundingSphereComponent>(entity);
        registry.emplace_or_replace<FVisibilityComponent>(entity, true);
        if (skinnedRoot != entt::null)
            registry.emplace_or_replace<FSkinnedRootComponent>(entity, skinnedRoot);
    }

    bool FEntityFactory::ApplyMeshKey(FEntitySubsystem& subsystem, IAssetRegistry& registry, entt::entity entity, FModelAssetHandle asset, int32_t nodeIndex, int32_t primitiveIndex)
    {
        auto& reg = subsystem.GetRegistry();
        const FMeshRendererComponent* mc = reg.try_get<FMeshRendererComponent>(entity);
        if (mc == nullptr || !asset.IsValid() || nodeIndex < 0 || primitiveIndex < 0)
            return false;

        const FMeshResourceHandle mh = registry.AcquireMesh(asset, static_cast<uint32_t>(nodeIndex), static_cast<uint32_t>(primitiveIndex));
        const FMaterialResourceHandle mat = registry.AcquireMaterial(asset, static_cast<uint32_t>(nodeIndex), static_cast<uint32_t>(primitiveIndex));
        if (!mh.IsValid() || !mat.IsValid())
        {
            if (mh.IsValid()) registry.Release(mh);
            if (mat.IsValid()) registry.Release(mat);
            return false;
        }

        const FMeshGPUData md = registry.GetMeshData(mh);
        if (md.bSkinned != mc->bSkinned || (md.bSkinned && !(asset == mc->Asset)))
        {
            registry.Release(mh);
            registry.Release(mat);
            VTNA_LOG_WARN("[EntityFactory] mesh swap rejected: skinned/static or cross-asset skinned (entity {})", entt::to_integral(entity));
            return false;
        }

        registry.Release(mc->MeshHandle);
        registry.Release(mc->MaterialHandle);
        SetMeshRenderer(reg, entity, mh, mat, md, asset, nodeIndex, primitiveIndex);
        subsystem.MarkTransformDirty(entity);
        return true;
    }

    bool FEntityFactory::ApplyMeshOverride(const FMeshOverride& meshOverride)
    {
        auto& reg = m_Subsystem.GetRegistry();
        if (meshOverride.SourceRoot == entt::null || !reg.valid(meshOverride.SourceRoot) ||
            meshOverride.SourceNodeIndex < 0 || meshOverride.SourcePrimitiveIndex < 0)
            return false;

        const FModelNodeTableComponent* table = reg.try_get<FModelNodeTableComponent>(meshOverride.SourceRoot);
        if (table == nullptr || static_cast<uint32_t>(meshOverride.SourceNodeIndex) >= table->NodeEntities.size())
            return false;

        const entt::entity nodeEntity = table->NodeEntities[static_cast<uint32_t>(meshOverride.SourceNodeIndex)];
        if (nodeEntity == entt::null || !reg.valid(nodeEntity))
            return false;

        const eastl::vector<entt::entity>* children = m_Subsystem.FindChildren(nodeEntity);
        if (children == nullptr)
            return false;

        for (entt::entity child : *children)
        {
            const FMeshRendererComponent* mc = reg.try_get<FMeshRendererComponent>(child);
            if (mc == nullptr)
                continue;
            if (mc->SourceAsset == meshOverride.SourceAsset &&
                mc->SourceNodeIndex == meshOverride.SourceNodeIndex &&
                mc->SourcePrimitiveIndex == meshOverride.SourcePrimitiveIndex)
            {
                return ApplyMeshKey(m_Subsystem, m_Registry, child, meshOverride.TargetAsset, meshOverride.TargetNodeIndex, meshOverride.TargetPrimitiveIndex);
            }
        }
        VTNA_LOG_WARN("[EntityFactory] mesh override target not found (model {}, node {}, prim {})", meshOverride.SourceAsset.Index, meshOverride.SourceNodeIndex, meshOverride.SourcePrimitiveIndex);
        return false;
    }
} // namespace Scene::ECS