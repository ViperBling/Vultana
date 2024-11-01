#pragma once

#include "Light.hpp"

namespace Scene
{
    class DirectionalLight : public ILight
    {
    public:
        virtual bool Create();
        virtual void Tick(float deltaTime);
    };
}