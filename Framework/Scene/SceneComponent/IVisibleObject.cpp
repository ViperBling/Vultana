#include "IVisibleObject.hpp"
#include "Utilities/GUIUtil.hpp"

namespace Scene
{
    void Scene::IVisibleObject::OnGUI()
    {
        GUICommand("Inspector", "Transform", [&]()
        {
            ImGui::DragFloat3("Position", (float*)&m_Position, 0.01f, -1e8, 1e8, "%.3f");

            float3 angles = RotationAngles(GetRotation());
            if (ImGui::DragFloat3("Rotation", (float*)&angles, 0.01f, -180.0f, 180.0f, "%.3f"))
            {
                SetRotation(RotationQuat(angles));
            }

            ImGui::DragFloat3("Scale", (float*)&m_Scale, 0.01f, 0.0f, 1e8, "%.3f");
        });
    }
}
