#include "World.hpp"
#include "SceneComponent/DirectionalLight.hpp"
#include "Renderer/RendererBase.hpp"
#include "Core/VultanaEngine.hpp"
#include "AssetManager/ModelLoader.hpp"

#include <atomic>

#include <tinyxml2/tinyxml2.h>

namespace Scene
{
    World::World()
    {
        mpCamera = std::make_unique<Camera>();
    }

    World::~World()
    {
    }

    void World::LoadScene(const std::string &file)
    {
        // TODO : Load scene from file
        CreateSceneObject(nullptr);
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
        CreateLight(element);
        CreateCamera(element);
        CreateModel(element);
    }

    void World::CreateLight(tinyxml2::XMLElement *element)
    {
        ILight* light = new DirectionalLight();

        if (!light->Create())
        {
            delete light;
            return;
        }

        light->SetLightColor({ 1.0f, 1.0f, 1.0f });
        light->SetLightDirection({ 0.0f, 1.0f, 0.0f });

        AddLight(light);

        if (mpMainLight == nullptr)
        {
            mpMainLight = light;
        }
    }

    void World::CreateCamera(tinyxml2::XMLElement *element)
    {
        mpCamera->SetPosition({ 0.0f, 0.0f, -3.0f });

        Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        uint32_t width = pRenderer->GetRenderWidth();
        uint32_t height = pRenderer->GetRenderHeight();
        mpCamera->SetPerspective(static_cast<float>(width) / height, 60.0f, 0.01f);
    }

    void World::CreateModel(tinyxml2::XMLElement *element)
    {
        Assets::ModelLoader loader(this);
        loader.LoadGLTF();
    }
}