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
        void AddLight(ILight* light);

        void Tick(float deltaTime);

        IVisibleObject* GetVisibleObject(uint32_t index) const;
        ILight* GetMainLight();

    private:
        void ClearScene();

        void CreateSceneObject(tinyxml2::XMLElement* element);
        void CreateLight(tinyxml2::XMLElement* element);
        void CreateCamera(tinyxml2::XMLElement* element);
        void CreateModel(tinyxml2::XMLElement* element);

    private:
        std::unique_ptr<Camera> mpCamera;

        std::vector<std::unique_ptr<IVisibleObject>> mObjects;
        std::vector<std::unique_ptr<ILight>> mLights;
        ILight* mpMainLight = nullptr;
    };
}