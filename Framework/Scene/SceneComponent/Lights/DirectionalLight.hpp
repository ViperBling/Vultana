#pragma once

#include "Light.hpp"

namespace Scene
{
    class FDirectionalLight : public ILight
    {
    public:
        virtual bool Create();
        virtual void Tick(float deltaTime);
    };
}