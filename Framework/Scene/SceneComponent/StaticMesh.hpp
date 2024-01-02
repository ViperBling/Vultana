#pragma once

#include <iostream>


#include "Primitive.hpp"

namespace Scene
{
    class StaticMesh : public IPrimitive
    {
    public:
        StaticMesh(const std::string& name);
        ~StaticMesh();

        virtual bool Create() override;
        virtual void Tick(float deltaTime) override;
        virtual void Render(Renderer::RendererBase* pRenderer) override;

    private:
        std::string& mName;
        Math::Vector3 mCenter = { 0.0f, 0.0f, 0.0f };
        float mRadius = 0.0f;
    };
} // namespace Vultana::Scene
