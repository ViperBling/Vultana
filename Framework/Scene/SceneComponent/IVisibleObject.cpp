#include "IVisibleObject.hpp"
#include "Utilities/GUIUtil.hpp"

namespace Scene
{
    void Scene::IVisibleObject::OnGUI()
    {
        GUI("Inspector", "Transform", [&]()
        {
            ImGui::DragFloat3("Position", (float*)&mPosition, 0.01f, -1e8, 1e8, "%.3f");
            ImGui::DragFloat4("Rotation", (float*)&mRotation, 0.01f, -180.0f, 180.0f, "%.3f");
            ImGui::DragFloat3("Scale", (float*)&mScale, 0.01f, 0.0f, 1e8, "%.3f");
        });
    }
}
