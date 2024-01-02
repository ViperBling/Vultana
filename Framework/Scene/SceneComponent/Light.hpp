#pragma once

#include "Primitive.hpp"

namespace Scene
{
    class ILight : public IPrimitive
    {
    public:
        virtual Math::Vector3 GetLightDir() const { return mLightDir; }
        virtual void SetLightDir(const Math::Vector3& lightDir) { mLightDir = lightDir; }

        virtual Math::Vector3 GetLightColor() const { return mLightColor; }
        virtual void SetLightColor(const Math::Vector3& lightColor) { mLightColor = lightColor; }

        virtual float GetLightIntensity() const { return mLightIntensity; }
        virtual void SetLightIntensity(float lightIntensity) { mLightIntensity = lightIntensity; }

    protected:
        Math::Vector3 mLightDir = { 1.0f, 1.0f, 1.0f };
        Math::Vector3 mLightColor = { 1.0f, 1.0f, 1.0f };
        float mLightIntensity = 1.0f;
    };
}