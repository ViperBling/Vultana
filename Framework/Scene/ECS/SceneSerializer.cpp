#include "Scene/ECS/SceneSerializer.hpp"
#include "Scene/ECS/Components.hpp"
#include "Scene/ECS/SceneLightSnapshot.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

#include <EASTL/hash_set.h>
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

    // New <ECSScene> stores the rotation as the raw quaternion: FTransform
    // is a float4, zero-loss. Euler degrees are only decoded on the legacy
    // <Scene> read path.
    void WriteTransform(tinyxml2::XMLElement* el, const FTransformComponent& tf)
    {
        el->SetAttribute("Position", Float3ToString(tf.Position).c_str());
        el->SetAttribute("RotationQuat", Float3ToString(float3(tf.Rotation.x, tf.Rotation.y, tf.Rotation.z)).c_str());
        el->SetAttribute("Scale", Float3ToString(tf.Scale).c_str());
    }

    void ReadTransform(const tinyxml2::XMLElement* el, FTransformComponent& tf)
    {
        if (const char* pos = el->Attribute("Position")) tf.Position = ParseFloat3(pos);
        if (const char* q = el->Attribute("RotationQuat"))
        {
            const float3 v = ParseFloat3(q);
            const float w = el->Attribute("RotationW") ? el->FloatAttribute("RotationW") : 1.0f;
            tf.Rotation = float4(v.x, v.y, v.z, w);
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
            const eastl::string type = (typeAttr != nullptr) ? eastl::string(typeAttr) : eastl::string(el->Name());
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

                if (const char* rot = el->Attribute("Rotation"))
                    lightComp.Direction = ComputeLightDirection(RotationQuat(ParseFloat3(rot)));
                if (el->Attribute("Radius"))
                    lightComp.Radius = el->FloatAttribute("Radius");

                reg.emplace<FLightComponent>(created, lightComp);
                reg.emplace<FMainLightTag>(created);
                if (auto* tf = reg.try_get<FTransformComponent>(created))
                    ReadTransform(el, *tf);
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

    bool Scene::ECS::FSceneSerializer::Save(FEntitySubsystem& subsystem, const eastl::string& filePath, const eastl::string& assetsPath)
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
