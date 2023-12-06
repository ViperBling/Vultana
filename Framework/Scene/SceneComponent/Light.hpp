#pragma once

#include "Primitive.hpp"

namespace Vultana
{
    class ILight : public IPrimitive
    {
    public:
        virtual Vector3 GetLightDir() const { return mLightDir; }
        virtual void SetLightDir(const Vector3& lightDir) { mLightDir = lightDir; }

        virtual Vector3 GetLightColor() const { return mLightColor; }
        virtual void SetLightColor(const Vector3& lightColor) { mLightColor = lightColor; }

        virtual float GetLightIntensity() const { return mLightIntensity; }
        virtual void SetLightIntensity(float lightIntensity) { mLightIntensity = lightIntensity; }

    protected:
        Vector3 mLightDir = { 1.0f, 1.0f, 1.0f };
        Vector3 mLightColor = { 1.0f, 1.0f, 1.0f };
        float mLightIntensity = 1.0f;
    };
}