#include "World.hpp"
#include "SceneComponent/DirectionalLight.hpp"
#include "Renderer/RendererBase.hpp"
#include "Core/VultanaEngine.hpp"
#include "AssetManager/ModelLoader.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"
#include "Utilities/String.hpp"

#include <atomic>

#include <tinyxml2/tinyxml2.h>

inline float3 strToFloat3(const std::string& str)
{
    std::vector<float> v;
    v.reserve(3);
    StringUtils::StringToFloatArray(str, v);
    return float3(v[0], v[1], v[2]);
}

namespace Scene
{
    inline void LoadVisibleObject(tinyxml2::XMLElement *element, IVisibleObject* object)
    {
        const tinyxml2::XMLAttribute* position = element->FindAttribute("Position");
        if (position)
        {
            object->SetPosition(strToFloat3(position->Value()));
        }
        const tinyxml2::XMLAttribute* rotation = element->FindAttribute("Rotation");
        if (rotation)
        {
            object->SetRotation(RotationQuat(strToFloat3(rotation->Value())));
        }
        const tinyxml2::XMLAttribute* scale = element->FindAttribute("Scale");
        if (scale)
        {
            object->SetScale(strToFloat3(scale->Value()));
        }
    }

    inline void LoadLight(tinyxml2::XMLElement *element, ILight* light)
    {
        LoadVisibleObject(element, light);

        const tinyxml2::XMLAttribute* color = element->FindAttribute("Color");
        if (color)
        {
            light->SetLightColor(strToFloat3(color->Value()));
        }
        const tinyxml2::XMLAttribute* intensity = element->FindAttribute("Intensity");
        if (intensity)
        {
            light->SetLightIntensity(std::stof(intensity->Value()));
        }
    }

    World::World()
    {
        mpCamera = std::make_unique<Camera>();
    }

    World::~World()
    {
    }

    void World::LoadScene(const std::string &file)
    {
        VTNA_LOG_INFO("Loading scene from file: {}", file);

        tinyxml2::XMLDocument xmlDoc;
        if (tinyxml2::XML_SUCCESS != xmlDoc.LoadFile(file.c_str()))
        {
            VTNA_LOG_ERROR("Failed to load scene file: {}", file);
            return;
        }
        ClearScene();

        tinyxml2::XMLNode* rootNode = xmlDoc.FirstChild();
        assert(rootNode != nullptr && strcmp(rootNode->Value(), "Scene") == 0);
        for (tinyxml2::XMLElement* element = rootNode->FirstChildElement(); element != nullptr; element = (tinyxml2::XMLElement*)element->NextSibling())
        {
            CreateSceneObject(element);
        }
    }

    void World::AddObject(IVisibleObject *object)
    {
        assert(object != nullptr);
        object->SetID(static_cast<uint32_t>(mObjects.size()));
        mObjects.push_back(std::unique_ptr<IVisibleObject>(object));
    }

    void World::AddLight(ILight *light)
    {
        assert(light != nullptr);
        mLights.push_back(std::unique_ptr<ILight>(light));
    }

    void World::Tick(float deltaTime)
    {
        mpCamera->Tick(deltaTime);

        for (auto iter = mObjects.begin(); iter != mObjects.end(); ++iter)
        {
            (*iter)->Tick(deltaTime);
        }

        for (auto iter = mLights.begin(); iter != mLights.end(); ++iter)
        {
            (*iter)->Tick(deltaTime);
        }

        Renderer::RendererBase* pRender = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        for (auto iter = mObjects.begin(); iter != mObjects.end(); ++iter)
        {
            (*iter)->Render(pRender);
        }
    }

    IVisibleObject *World::GetVisibleObject(uint32_t index) const
    {
        if (index >= mObjects.size())
        {
            return nullptr;
        }
        return mObjects[index].get();
    }

    ILight *World::GetMainLight()
    {
        assert(mpMainLight != nullptr);
        return mpMainLight;
    }

    void World::ClearScene()
    {
        mObjects.clear();
        mLights.clear();
        mpMainLight = nullptr;
    }

    void World::CreateSceneObject(tinyxml2::XMLElement *element)
    {
        if (strcmp(element->Value(), "Light") == 0)
        {
            CreateLight(element);
        }
        else if (strcmp(element->Value(), "Camera") == 0)
        {
            CreateCamera(element);
        }
        else if (strcmp(element->Value(), "Model") == 0)
        {
            CreateModel(element);
        }
    }

    void World::CreateLight(tinyxml2::XMLElement *element)
    {
        ILight* light = nullptr;

        const tinyxml2::XMLAttribute* type = element->FindAttribute("Type");
        assert(type != nullptr);
        if (strcmp(type->Value(), "Directional") == 0)
        {
            light = new DirectionalLight();
        }
        else
        {
            // TODO
            // light = new DirectionalLight();
            assert(false);
        }
        LoadLight(element, light);
        if (!light->Create())
        {
            delete light;
            return;
        }

        AddLight(light);

        const tinyxml2::XMLAttribute* mainLight = element->FindAttribute("IsMainLight");
        if (mainLight && mainLight->BoolValue())
        {
            mpMainLight = light;
        }
    }

    void World::CreateCamera(tinyxml2::XMLElement *element)
    {
        const tinyxml2::XMLAttribute* position = element->FindAttribute("Position");
        if (position)
        {
            mpCamera->SetPosition(strToFloat3(position->Value()));
        }
        const tinyxml2::XMLAttribute* rotation = element->FindAttribute("Rotation");
        if (rotation)
        {
            mpCamera->SetRotation(strToFloat3(rotation->Value()));
        }

        Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        uint32_t width = pRenderer->GetRenderWidth();
        uint32_t height = pRenderer->GetRenderHeight();
        mpCamera->SetPerspective(static_cast<float>(width) / height, element->FindAttribute("Fov")->FloatValue(), element->FindAttribute("ZNear")->FloatValue());
    }

    void World::CreateModel(tinyxml2::XMLElement *element)
    {
        Assets::ModelLoader loader(this);
        loader.LoadModelSettings(element);
        loader.LoadGLTF();
    }
}