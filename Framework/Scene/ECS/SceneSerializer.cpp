#include "Scene/ECS/SceneSerializer.hpp"
#include "Scene/ECS/Components.hpp"
#include "Scene/ECS/SceneLightSnapshot.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

#include <EASTL/hash_set.h>
#include <EASTL/hash_map.h>
#include <EASTL/functional.h>

#include <tinyxml2/tinyxml2.h>
#include <cstdio>

namespace Scene::ECS
{
    float3 ParseFloat3(const char* value)
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        sscanf(value, "%f, %f, %f", &x, &y, &z);
        return float3(x, y, z);
    }

    eastl::string Float3ToString(const float3& v)
    {
        char buf[96];
        snprintf(buf, sizeof(buf), "%.6f, %.6f, %.6f", v.x, v.y, v.z);
        return eastl::string(buf);
    }

    float4 ParseFloat4(const char* value)
    {
        float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
        sscanf(value, "%f, %f, %f, %f", &x, &y, &z, &w);
        return float4(x, y, z, w);
    }

    eastl::string Float4ToString(const float4& v)
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "%.6f, %.6f, %.6f, %.6f", v.x, v.y, v.z, v.w);
        return eastl::string(buf);
    }

    void WriteTransform(tinyxml2::XMLElement* el, const FTransformComponent& tf)
    {
        el->SetAttribute("Position", Float3ToString(tf.Position).c_str());
        el->SetAttribute("RotationQuat", Float4ToString(tf.Rotation).c_str());
        el->SetAttribute("Scale", Float3ToString(tf.Scale).c_str());
    }

    void ReadTransform(const tinyxml2::XMLElement* el, FTransformComponent& tf)
    {
        if (const char* pos = el->Attribute("Position")) tf.Position = ParseFloat3(pos);
        if (const char* q = el->Attribute("RotationQuat"))
        {
            tf.Rotation = ParseFloat4(q);
        }
        else if (const char* rot = el->Attribute("Rotation")) // legacy euler degrees
        {
            tf.Rotation = float4(RotationQuat(ParseFloat3(rot)));
        }
        if (const char* scale = el->Attribute("Scale")) tf.Scale = ParseFloat3(scale);
    }

    bool IsUserEntity(const entt::registry& reg, entt::entity e, const eastl::hash_set<entt::id_type>& derivedIds)
    {
        if (!reg.all_of<FTransformComponent>(e))
            return false;
        if (reg.all_of<FMeshRendererComponent>(e)) // derived mesh entity
            return false;
        if (derivedIds.contains(entt::to_integral(e))) // derived node entity
            return false;
        return true;
    }

    eastl::string ToRelativePath(const eastl::string& fullPath, const eastl::string& assetsPath)
    {
        if (!assetsPath.empty() && fullPath.length() >= assetsPath.length() &&
            fullPath.compare(0, assetsPath.length(), assetsPath) == 0)
            return eastl::string(fullPath.begin() + assetsPath.length(), fullPath.end());
        return fullPath;
    }

    bool Scene::ECS::FSceneSerializer::Load(FEntitySubsystem& subsystem, FEntityFactory& factory, IAssetRegistry& assetRegistry, const eastl::string& filePath, const eastl::string& assetsPath, FSceneLoadResult& outResult)
    {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(filePath.c_str()) != tinyxml2::XML_SUCCESS)
        {
            VTNA_LOG_ERROR("[SceneSerializer] failed to load scene file: {}", filePath);
            return false;
        }

        const tinyxml2::XMLElement* rootNode = doc.RootElement();
        if (rootNode == nullptr) return false;
        const char* rootName = rootNode->Name();
        if (rootName == nullptr || (strcmp(rootName, "Scene") != 0 && strcmp(rootName, "ECSScene") != 0))
        {
            VTNA_LOG_ERROR("[SceneSerializer] unexpected scene root element: {}", rootName ? rootName : "(null)");
            return false;
        }

        eastl::function<entt::entity(const tinyxml2::XMLElement*, entt::entity)> parseElement;
        parseElement = [&](const tinyxml2::XMLElement* el, entt::entity parent) -> entt::entity
        {
            const char* typeAttr = el->Attribute("Type");
            const bool bTaggedEntity = (strcmp(el->Name(), "Entity") == 0);
            const eastl::string type = (bTaggedEntity && typeAttr != nullptr) ? eastl::string(typeAttr)
                                                                              : eastl::string(el->Name());
            const char* nameAttr = el->Attribute("Name");
            entt::entity created = entt::null;

            if (type == "Model")
            {
                const char* fileAttr = el->Attribute("File");
                if (fileAttr ==  nullptr) return entt::null;

                const eastl::string fullPath = assetsPath + fileAttr;
                FModelAssetHandle assetHandle = assetRegistry.ImportModel(fullPath);
                if (!assetHandle.IsValid())
                {
                    VTNA_LOG_ERROR("[SceneSerializer] ImportModel failed: {}", fullPath);
                    return entt::null;
                }
                created = factory.CreateModelInstance(assetHandle, fullPath);
                if (nameAttr != nullptr)
                {
                    if (auto* nameComp = subsystem.GetRegistry().try_get<FNameComponent>(created))
                        nameComp->SetName(nameAttr);
                }

                FModelLoadRequest request;
                request.Handle = assetHandle;
                request.Root = created;
                request.FilePath = fullPath;
                if (auto* tf = subsystem.GetRegistry().try_get<FTransformComponent>(created))
                {
                    ReadTransform(el, *tf);
                    const quaternion q (tf->Rotation.x, tf->Rotation.y, tf->Rotation.z, tf->Rotation.w);
                    request.Position = tf->Position;
                    request.Rotation = q;
                    request.Scale = tf->Scale;
                }
                outResult.Models.push_back(eastl::move(request));

                for (const tinyxml2::XMLElement* ovEl = el->FirstChildElement("MeshOverride"); ovEl != nullptr; ovEl = ovEl->NextSiblingElement("MeshOverride"))
                {
                    const int32_t srcNode = ovEl->IntAttribute("SourceNode", -1);
                    const int32_t srcPrim = ovEl->IntAttribute("SourcePrimitive", -1);
                    const int32_t tgtNode = ovEl->IntAttribute("Node", -1);
                    const int32_t tgtPrim = ovEl->IntAttribute("Primitive", -1);
                    const char* overrideFileAttr = ovEl->Attribute("File");
                    if (srcNode < 0 || srcPrim < 0 || tgtNode < 0 || tgtPrim < 0 || overrideFileAttr == nullptr)
                    {
                        VTNA_LOG_WARN("[SceneSerializer] malformed MeshOverride under model {}; skipped", fullPath);
                        continue;
                    }
                    const eastl::string targetPath = assetsPath + overrideFileAttr;
                    const FModelAssetHandle targetHandle = assetRegistry.ImportModel(targetPath);
                    if (!targetHandle.IsValid())
                    {
                        VTNA_LOG_WARN("[SceneSerializer] MeshOverride target import failed: {}", targetPath);
                        continue;
                    }
                    FMeshOverride meshOverride;
                    meshOverride.SourceAsset = assetHandle;
                    meshOverride.SourceRoot = created;
                    meshOverride.SourceNodeIndex = srcNode;
                    meshOverride.SourcePrimitiveIndex = srcPrim;
                    meshOverride.TargetAsset = targetHandle;
                    meshOverride.TargetNodeIndex = tgtNode;
                    meshOverride.TargetPrimitiveIndex = tgtPrim;
                    outResult.MeshOverrides.push_back(eastl::move(meshOverride));

                    if (targetHandle != assetHandle && !assetRegistry.IsReady(targetHandle))
                    {
                        bool bAlreadyPreloaded = false;
                        for (const FModelPreloadRequest& existing : outResult.ModelPreloads)
                        {
                            if (existing.Handle == targetHandle) { bAlreadyPreloaded = true; break; }
                        }
                        if (!bAlreadyPreloaded)
                        {
                            FModelPreloadRequest preload;
                            preload.Handle = targetHandle;
                            preload.FilePath = targetPath;
                            outResult.ModelPreloads.push_back(eastl::move(preload));
                        }
                    }
                }
            }
            else if (type == "Camera")
            {
                created = factory.CreateEmpty(nameAttr ? nameAttr : "MainCamera");
                auto& registry = subsystem.GetRegistry();
                registry.emplace<FCameraComponent>(created,
                    el->Attribute("Fov") ? el->FloatAttribute("Fov") : 60.0f,
                    el->Attribute("ZNear") ? el->FloatAttribute("ZNear") : 0.01f,
                    el->Attribute("ZFar") ? el->FloatAttribute("ZFar") : 1000.0f);
                registry.emplace<FMainCameraTag>(created);
                if (auto* tf = registry.try_get<FTransformComponent>(created))
                    ReadTransform(el, *tf);
                outResult.bHasCamera = true;

                if (auto* tf = registry.try_get<FTransformComponent>(created))
                {
                    outResult.CameraPosition = tf->Position;
                    const quaternion q(tf->Rotation.x, tf->Rotation.y, tf->Rotation.z, tf->Rotation.w);
                    outResult.CameraRotation = RotationAngles(q);
                }
                if (auto* cam = registry.try_get<FCameraComponent>(created))
                {
                    outResult.CameraFov = cam->Fov;
                    outResult.CameraZNear = cam->ZNear;
                }
            }
            else if (type == "Light")
            {
                created = factory.CreateEmpty(nameAttr ? nameAttr : "MainLight");
                auto& reg = subsystem.GetRegistry();
                FLightComponent lightComp;
                if (const char* color = el->Attribute("Color"))
                    lightComp.Color = ParseFloat3(color);
                if (el->Attribute("Radius"))
                    lightComp.Radius = el->FloatAttribute("Radius");

                if (auto* tf = reg.try_get<FTransformComponent>(created))
                {
                    ReadTransform(el, *tf);
                    const quaternion q(tf->Rotation.x, tf->Rotation.y, tf->Rotation.z, tf->Rotation.w);
                    lightComp.Direction = ComputeLightDirection(q);
                }

                reg.emplace<FLightComponent>(created, lightComp);
                reg.emplace<FMainLightTag>(created);
            }
            else
            {
                created = factory.CreateEmpty(nameAttr ? nameAttr : "Entity");
                if (auto* tf = subsystem.GetRegistry().try_get<FTransformComponent>(created))
                    ReadTransform(el, *tf);
            }

            if (created != entt::null && parent != entt::null)
                subsystem.SetParent(created, parent);

            for (const tinyxml2::XMLElement* child = el->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
            {
                if (strcmp(child->Name(), "MeshOverride") == 0)
                    continue;
                parseElement(child, created);
            }
            return created;
        };
        for (const tinyxml2::XMLElement* element = rootNode->FirstChildElement(); element != nullptr; element = element->NextSiblingElement())
        {
            parseElement(element, entt::null);
        }
        return true;
    }

    bool Scene::ECS::FSceneSerializer::Save(FEntitySubsystem& subsystem, IAssetRegistry& assetRegistry, const eastl::string& filePath, const eastl::string& assetsPath)
    {
        auto& registry = subsystem.GetRegistry();

        eastl::hash_set<entt::id_type> derivedIds;
        registry.view<FModelNodeTableComponent>().each([&](entt::entity, const FModelNodeTableComponent& table)
        {
            for (entt::entity n : table.NodeEntities)
            {
                if (n != entt::null)
                    derivedIds.insert(entt::to_integral(n));
            }
        });

        constexpr uint32_t MAX_ANCESTOR_DEPTH = 4096;
        eastl::hash_map<entt::id_type, eastl::vector<entt::entity>> editedMeshesByModel;
        registry.view<FMeshRendererComponent>().each([&](entt::entity meshEntity, const FMeshRendererComponent& mc)
        {
            if (mc.Asset == mc.SourceAsset && mc.NodeIndex == mc.SourceNodeIndex && mc.PrimitiveIndex == mc.SourcePrimitiveIndex)
                return;   // not edited
            entt::entity owner = entt::null;
            entt::entity cur = meshEntity;
            for (uint32_t depth = 0; cur != entt::null && registry.valid(cur) && depth < MAX_ANCESTOR_DEPTH; ++depth)
            {
                if (registry.all_of<FModelInstanceComponent>(cur)) { owner = cur; break; }
                const auto* parentComp = registry.try_get<FParentComponent>(cur);
                cur = (parentComp != nullptr) ? parentComp->Parent : entt::null;
            }
            if (owner == entt::null)
            {
                VTNA_LOG_WARN("[SceneSerializer] edited mesh entity {} has no model instance ancestor; override not saved", entt::to_integral(meshEntity));
                return;
            }
            editedMeshesByModel[entt::to_integral(owner)].push_back(meshEntity);
        });

        tinyxml2::XMLDocument doc;
        tinyxml2::XMLElement* root = doc.NewElement("ECSScene");
        root->SetAttribute("Version", 1);
        doc.InsertEndChild(root);

        eastl::function<void(tinyxml2::XMLElement*, entt::entity)> emit;
        emit = [&](tinyxml2::XMLElement* parentEl, entt::entity e)
        {
            auto* element = doc.NewElement("Entity");
            parentEl->InsertEndChild(element);

            if (const auto* name = registry.try_get<FNameComponent>(e))
                element->SetAttribute("Name", name->Name);
            
            const auto* tf = registry.try_get<FTransformComponent>(e);
            if (registry.all_of<FMainCameraTag>(e))
            {
                element->SetAttribute("Type", "Camera");
                if (const auto* cam = registry.try_get<FCameraComponent>(e))
                {
                    element->SetAttribute("Fov", cam->Fov);
                    element->SetAttribute("ZNear", cam->ZNear);
                    element->SetAttribute("ZFar", cam->ZFar);
                }
                if (tf) WriteTransform(element, *tf);
            }
            else if (registry.all_of<FMainLightTag>(e))
            {
                element->SetAttribute("Type", "Light");
                if (const auto* light = registry.try_get<FLightComponent>(e))
                {
                    element->SetAttribute("Color", Float3ToString(light->Color).c_str());
                    element->SetAttribute("Radius", light->Radius);
                }
                // Direction is derived from Rotation on load: write the entity rotation via WriteTransform, no Direction attribute.
                if (tf) WriteTransform(element, *tf);
            }
            else if (const auto* model = registry.try_get<FModelInstanceComponent>(e))
            {
                element->SetAttribute("Type", "Model");
                element->SetAttribute("File", ToRelativePath(model->ModelPath, assetsPath).c_str());
                if (tf) WriteTransform(element, *tf);

                if (const auto editedIt = editedMeshesByModel.find(entt::to_integral(e)); editedIt != editedMeshesByModel.end())
                {
                    for (entt::entity meshEntity : editedIt->second)
                    {
                        const FMeshRendererComponent& mc = registry.get<FMeshRendererComponent>(meshEntity);
                        if (!mc.Asset.IsValid())
                        {
                            VTNA_LOG_WARN("[SceneSerializer] mesh override with invalid target asset skipped (entity {})", entt::to_integral(meshEntity));
                            continue;
                        }
                        const FModelAssetInfo targetInfo = assetRegistry.GetModelInfo(mc.Asset);
                        if (targetInfo.ModelPath.empty())
                        {
                            VTNA_LOG_WARN("[SceneSerializer] mesh override target path empty; skipped (entity {})", entt::to_integral(meshEntity));
                            continue;
                        }
                        auto* ovEl = doc.NewElement("MeshOverride");
                        ovEl->SetAttribute("SourceNode", mc.SourceNodeIndex);
                        ovEl->SetAttribute("SourcePrimitive", mc.SourcePrimitiveIndex);
                        ovEl->SetAttribute("File", ToRelativePath(targetInfo.ModelPath, assetsPath).c_str());
                        ovEl->SetAttribute("Node", mc.NodeIndex);
                        ovEl->SetAttribute("Primitive", mc.PrimitiveIndex);
                        element->InsertEndChild(ovEl);
                    }
                }
            }
            else
            {
                element->SetAttribute("Type", "Empty");
                if (tf) WriteTransform(element, *tf);
            }

            if (const auto* children = subsystem.FindChildren(e))
            {
                for (entt::entity c : *children)
                {
                    if (IsUserEntity(registry, c, derivedIds))
                        emit(element, c);
                }
            }
        };

        // Emit each user entity whose parent is not itself user-editable (root of the
        // editable forest); children nest inside, matching the Outliner tree.
        for (entt::entity e : registry.view<FTransformComponent>())
        {
            if (!IsUserEntity(registry, e, derivedIds))
                continue;
            const auto* parent = registry.try_get<FParentComponent>(e);
            if (parent != nullptr && parent->Parent != entt::null && registry.valid(parent->Parent) && IsUserEntity(registry, parent->Parent, derivedIds))
                continue;
            if (parent != nullptr && parent->Parent != entt::null && registry.valid(parent->Parent) && !IsUserEntity(registry, parent->Parent, derivedIds))
            {
                VTNA_LOG_WARN("[SceneSerializer] entity '{}' is parented under a derived node; parent link will be lost on save/reload", registry.all_of<FNameComponent>(e) ? registry.get<FNameComponent>(e).Name : "(unnamed)");
            }
            emit(root, e);
        }

        const tinyxml2::XMLError result = doc.SaveFile(filePath.c_str());
        if (result != tinyxml2::XML_SUCCESS)
        {
            VTNA_LOG_ERROR("[SceneSerializer] failed to save scene file: {} ({})", filePath, static_cast<int>(result));
            return false;
        }
        return true;
    }
}
