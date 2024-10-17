#pragma once

#include "IVisibleObject.hpp"

namespace Scene
{
    class ILight : public IVisibleObject
    {
    public:
        float3 GetLightDirection() const { return mLightDirection; }
        void SetLightDirection(const float3& direction) { mLightDirection = direction; }

        float3 GetLightColor() const { return mLightColor; }
        void SetLightColor(const float3& color) { mLightColor = color; }

        float GetLightIntensity() const { return mLightIntensity; }
        void SetLightIntensity(float intensity) { mLightIntensity = intensity; }

        float GetLightRadius() const { return mLightRadius; }
        void SetLightRadius(float radius) { mLightRadius = radius; }

    protected:
        float3 mLightDirection = { 0.0f, 1.0f, 0.0f };
        float3 mLightColor = { 1.0f, 1.0f, 1.0f };
        float mLightIntensity = 1.0f;
        float mLightRadius = 0.005f;
    };
}