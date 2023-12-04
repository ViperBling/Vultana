#include "World.hpp"
#include "Renderer/Renderer.hpp"
#include "Core/VultanaEngine.hpp"

#include <atomic>

#include <tinyxml2/tinyxml2.h>

using namespace Vultana;

namespace Vultana::Scene
{
    World::World()
    {
        Renderer::RendererBase* pRender = Engine::GetEngineInstance()->GetRenderer();

        mpCamera = std::make_unique<Camera>();
    }

    World::~World()
    {
    }

    void World::Tick(float deltaTime)
    {
        mpCamera->Tick(deltaTime);

        Renderer::RendererBase* pRender = Engine::GetEngineInstance()->GetRenderer();

        for (auto it = mPrimitives.begin(); it != mPrimitives.end(); ++it)
        {
            (*it)->Tick(deltaTime);
        }
        for (auto it = mLights.begin(); it != mLights.end(); ++it)
        {
            (*it)->Tick(deltaTime);
        }

        for (auto it = mPrimitives.begin(); it != mPrimitives.end(); ++it)
        {
            (*it)->Render(pRender);
        }
    }

    void World::AddPrimitive(IPrimitive *primitive)
    {
        assert(primitive != nullptr);
        mPrimitives.push_back(std::unique_ptr<IPrimitive>(primitive));
    }

    void World::AddLight(ILight *light)
    {
        assert(light != nullptr);
        mLights.push_back(std::unique_ptr<ILight>(light));
    }

    void World::LoadScene(const std::string &file)
    {
    }

    void World::SaveScene(const std::string &file)
    {
    }

    IPrimitive *World::GetPrimitive(uint32_t id) const
    {
        if (id >= mPrimitives.size())
        {
            return nullptr;
        }
        return mPrimitives[id].get();
    }

    ILight *World::GetPrimaryLight() const
    {
        assert(mpPrimaryLight != nullptr);
        return mpPrimaryLight;
    }

    void World::ClearScene()
    {
        mPrimitives.clear();
        mLights.clear();
    }

    void World::CreatePrimitive(tinyxml2::XMLElement *element)
    {
        if (strcmp(element->Value(), "light") == 0)
        {
            CreateLight(element);
        }
        else if (strcmp(element->Value(), "camera") == 0)
        {
            CreateCamera(element);
        }
        else if (strcmp(element->Value(), "model") == 0)
        {
            CreateModel(element);
        }
    }

    void World::CreateLight(tinyxml2::XMLElement *element)
    {
        
    }

    void World::CreateCamera(tinyxml2::XMLElement *element)
    {
    }

    void World::CreateModel(tinyxml2::XMLElement *element)
    {
    }
}