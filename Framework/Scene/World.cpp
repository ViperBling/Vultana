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
        mpCamera->SetPosition({ 0.0f, 0.0f, -1.0f });
        mpCamera->SetRotation({ 0.0f, 0.0f, 0.0f });

        Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        uint32_t width = pRenderer->GetRenderWidth();
        uint32_t height = pRenderer->GetRenderHeight();
        mpCamera->SetPerspective(static_cast<float>(width) / height, 60.0f, 0.01f);
    }

    void World::CreateModel(tinyxml2::XMLElement *element)
    {
    }
}