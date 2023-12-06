#pragma once

#include "Utilities/Math.hpp"
#include "Renderer/Renderer.hpp"

#include <vector>

namespace Vultana
{
    class IPrimitive
    {
    public:
        virtual ~IPrimitive() {}

        virtual bool Create() = 0;
        virtual void Tick(float deltaTime) = 0;
        virtual void Render(RendererBase* pRenderer) {};

        virtual Vector3 GetPosition() const { return mPosition; }
        virtual void SetPosition(const Vector3& position) { mPosition = position; }
        virtual Vector3 GetRotation() const { return mRotation; }
        virtual void SetRotation(const Vector3& rotation) { mRotation = rotation; }
        virtual Vector3 GetScale() const { return mScale; }
        virtual void SetScale(const Vector3& scale) { mScale = scale; }

        virtual uint32_t GetID() const { return mID; }
        virtual void SetID(uint32_t id) { mID = id; }

    protected:
        uint32_t mID = 0;
        Vector3 mPosition = { 0.0f, 0.0f, 0.0f };
        Vector3 mRotation = { 0.0f, 0.0f, 0.0f };
        Vector3 mScale = { 1.0f, 1.0f, 1.0f };
    };
} // namespace Vultana