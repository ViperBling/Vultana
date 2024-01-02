#pragma once

#include "Utilities/Math.hpp"
#include "Renderer/Renderer.hpp"

#include <vector>

namespace Scene
{
    class IPrimitive
    {
    public:
        virtual ~IPrimitive() {}

        virtual bool Create() = 0;
        virtual void Tick(float deltaTime) = 0;
        virtual void Render(Renderer::RendererBase* pRenderer) {};

        virtual Math::Vector3 GetPosition() const { return mPosition; }
        virtual void SetPosition(const Math::Vector3& position) { mPosition = position; }
        virtual Math::Vector3 GetRotation() const { return mRotation; }
        virtual void SetRotation(const Math::Vector3& rotation) { mRotation = rotation; }
        virtual Math::Vector3 GetScale() const { return mScale; }
        virtual void SetScale(const Math::Vector3& scale) { mScale = scale; }

        virtual uint32_t GetID() const { return mID; }
        virtual void SetID(uint32_t id) { mID = id; }

    protected:
        uint32_t mID = 0;
        Math::Vector3 mPosition = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 mRotation = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 mScale = { 1.0f, 1.0f, 1.0f };
    };
} // namespace Vultana