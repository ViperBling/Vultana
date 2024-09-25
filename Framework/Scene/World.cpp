#include "World.hpp"
#include "Renderer/RendererBase.hpp"
#include "Core/VultanaEngine.hpp"

#include <atomic>

#include <tinyxml2/tinyxml2.h>

namespace Scene
{
    World::World()
    {
        Renderer::RendererBase* pRender = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

        mpCamera = std::make_unique<Camera>();
    }

    World::~World()
    {
    }

    void World::Tick(float deltaTime)
    {
        mpCamera->Tick(deltaTime);

        Renderer::RendererBase* pRender = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

        
    }

    

    void World::ClearScene()
    {
    }

    void World::CreatePrimitive(tinyxml2::XMLElement *element)
    {
        
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