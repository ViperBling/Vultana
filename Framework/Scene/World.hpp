#pragma once

#include "SceneComponent/Light.hpp"
#include "SceneComponent/StaticMesh.hpp"
#include "Camera.hpp"

namespace tinyxml2
{
    class XMLElement;
}

namespace Vultana::Scene
{
    class World
    {
    public:
        World();
        ~World();

        void Tick(float deltaTime);

        void AddPrimitive(IPrimitive* primitive);
        void AddLight(ILight* light);

        void LoadScene(const std::string& file);
        void SaveScene(const std::string& file);

        Camera* GetCamera() const { return mpCamera.get(); }
        IPrimitive* GetPrimitive(uint32_t id) const;
        ILight* GetPrimaryLight() const;

    private:
        void ClearScene();

        void CreatePrimitive(tinyxml2::XMLElement* element);
        void CreateLight(tinyxml2::XMLElement* element);
        void CreateCamera(tinyxml2::XMLElement* element);
        void CreateModel(tinyxml2::XMLElement* element);

    private:
        std::unique_ptr<Camera> mpCamera;

        std::vector<std::unique_ptr<IPrimitive>> mPrimitives;
        std::vector<std::unique_ptr<ILight>> mLights;

        ILight* mpPrimaryLight = nullptr;
    };
}