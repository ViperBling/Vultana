#pragma once

#include "Scene/SceneComponent/IVisibleObject.hpp"

namespace Scene
{
    class ILight : public IVisibleObject
    {
    public:
        float3 GetLightDirection() const { return m_LightDirection; }
        void SetLightDirection(const float3& direction) { m_LightDirection = direction; }

        float3 GetLightColor() const { return m_LightColor; }
        void SetLightColor(const float3& color) { m_LightColor = color; }

        float GetLightIntensity() const { return m_LightIntensity; }
        void SetLightIntensity(float intensity) { m_LightIntensity = intensity; }

        float GetLightRadius() const { return m_LightRadius; }
        void SetLightRadius(float radius) { m_LightRadius = radius; }

    protected:
        float3 m_LightDirection = { 0.0f, 1.0f, 0.0f };
        float3 m_LightColor = { 1.0f, 1.0f, 1.0f };
        float m_LightIntensity = 1.0f;
        float m_LightRadius = 0.005f;
    };
}