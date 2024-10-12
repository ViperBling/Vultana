#pragma once

#include "SceneComponent/Light.hpp"
#include "SceneComponent/StaticMesh.hpp"
#include "Camera.hpp"

namespace tinyxml2
{
    class XMLElement;
}

namespace Scene
{
    class World
    {
    public:
        World();
        ~World();

        Camera* GetCamera() { return mpCamera.get(); }

        void LoadScene(const std::string& file);

        void AddObject(IVisibleObject* object);

        void Tick(float deltaTime);

    private:
        void ClearScene();

        void CreateSceneObject(tinyxml2::XMLElement* element);
        void CreateLight(tinyxml2::XMLElement* element);
        void CreateCamera(tinyxml2::XMLElement* element);
        void CreateModel(tinyxml2::XMLElement* element);

    private:
        std::unique_ptr<Camera> mpCamera;

        std::list<std::unique_ptr<IVisibleObject>> mObjects;
        // std::list<std::unique_ptr<Light>> mLights;
    };
}