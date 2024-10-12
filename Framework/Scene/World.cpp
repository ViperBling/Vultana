#include "World.hpp"
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
        CreateCamera(nullptr);
        CreateModel(nullptr);
    }

    void World::AddObject(IVisibleObject *object)
    {
        assert(object != nullptr);
        mObjects.push_back(std::unique_ptr<IVisibleObject>(object));
    }

    void World::Tick(float deltaTime)
    {
        mpCamera->Tick(deltaTime);

        for (auto iter = mObjects.begin(); iter != mObjects.end(); ++iter)
        {
            (*iter)->Tick(deltaTime);
        }

        Renderer::RendererBase* pRender = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        for (auto iter = mObjects.begin(); iter != mObjects.end(); ++iter)
        {
            (*iter)->Render(pRender);
        }
    }

    void World::ClearScene()
    {
        mObjects.clear();
    }

    void World::CreateSceneObject(tinyxml2::XMLElement *element)
    {
        
    }

    void World::CreateLight(tinyxml2::XMLElement *element)
    {
        
    }

    void World::CreateCamera(tinyxml2::XMLElement *element)
    {
        mpCamera->SetPosition({ 0.5f, 1.0f, -1.0f });
        mpCamera->SetRotation({ 60.0f, 10.0f, 0.0f });

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