#include "World.hpp"
#include "SceneComponent/Lights/DirectionalLight.hpp"
#include "Renderer/RendererBase.hpp"
#include "Core/VultanaEngine.hpp"
#include "AssetManager/ModelLoader.hpp"
#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"
#include "Utilities/String.hpp"
#include "Utilities/ParallelFor.hpp"
#include "Utilities/GUIUtil.hpp"

#include <EASTL/atomic.h>

#include <tinyxml2/tinyxml2.h>

inline float3 strToFloat3(const eastl::string& str)
{
    eastl::vector<float> v;
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
            light->SetLightIntensity(intensity->FloatValue());
        }
    }

    World::World()
    {
        m_pCamera = eastl::make_unique<Camera>();
    }

    World::~World()
    {
    }

    void World::LoadScene(const eastl::string &file)
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
        object->SetID(static_cast<uint32_t>(m_Objects.size()));
        m_Objects.push_back(eastl::unique_ptr<IVisibleObject>(object));
    }

    void World::AddLight(ILight *light)
    {
        assert(light != nullptr);
        m_Lights.push_back(eastl::unique_ptr<ILight>(light));
    }

    void World::OnGUI()
    {
        // GUICommand("WorldOutliner", "World", [&]()
        // {
            ImGui::Text("World Outliner");
            for (auto iter = m_Objects.begin(); iter != m_Objects.end(); ++iter)
            {
                ImGui::Text((*iter)->GetName().c_str());
            }
        // });
    }

    void World::Tick(float deltaTime)
    {
        m_pCamera->Tick(deltaTime);

        for (auto iter = m_Objects.begin(); iter != m_Objects.end(); ++iter)
        {
            (*iter)->Tick(deltaTime);
        }

        for (auto iter = m_Lights.begin(); iter != m_Lights.end(); ++iter)
        {
            (*iter)->Tick(deltaTime);
        }

        #pragma region : Frustum Culling
        eastl::vector<IVisibleObject*> visibleObjects(m_Objects.size());
        eastl::atomic<uint32_t> visibleCount = 0;
        
        Utilities::ParallelFor((uint32_t)m_Objects.size(), [&](uint32_t i)
        {
            if (m_Objects[i]->FrustumCull(m_pCamera->GetFrustumPlanes(), 6))
            {
                uint32_t idx = visibleCount.fetch_add(1);
                visibleObjects[idx] = m_Objects[i].get();
            }
        });
        visibleObjects.resize(visibleCount);

        Renderer::RendererBase* pRender = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        for (auto iter = visibleObjects.begin(); iter != visibleObjects.end(); ++iter)
        {
            (*iter)->Render(pRender);
        }
        #pragma endregion : Frustum Culling
    }

    IVisibleObject *World::GetVisibleObject(uint32_t index) const
    {
        if (index >= m_Objects.size())
        {
            return nullptr;
        }
        return m_Objects[index].get();
    }

    ILight *World::GetMainLight()
    {
        assert(m_pMainLight != nullptr);
        return m_pMainLight;
    }

    void World::ClearScene()
    {
        m_Objects.clear();
        m_Lights.clear();
        m_pMainLight = nullptr;
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
            m_pMainLight = light;
        }
    }

    void World::CreateCamera(tinyxml2::XMLElement *element)
    {
        const tinyxml2::XMLAttribute* position = element->FindAttribute("Position");
        if (position)
        {
            m_pCamera->SetPosition(strToFloat3(position->Value()));
        }
        const tinyxml2::XMLAttribute* rotation = element->FindAttribute("Rotation");
        if (rotation)
        {
            m_pCamera->SetRotation(strToFloat3(rotation->Value()));
        }

        Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        uint32_t width = pRenderer->GetRenderWidth();
        uint32_t height = pRenderer->GetRenderHeight();
        m_pCamera->SetPerspective(static_cast<float>(width) / height, element->FindAttribute("Fov")->FloatValue(), element->FindAttribute("ZNear")->FloatValue());
    }

    void World::CreateModel(tinyxml2::XMLElement *element)
    {
        Assets::ModelLoader loader(this);
        loader.LoadModelSettings(element);
        loader.LoadGLTF();
    }
}