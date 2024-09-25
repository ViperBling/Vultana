#pragma once

#include "Utilities/Math.hpp"
#include "Renderer/RendererBase.hpp"

#include <vector>

namespace Scene
{
    class IVisibleObject
    {
    public:
        virtual ~IVisibleObject() = default;

        virtual bool Create() = 0;
        virtual void Tick(float deltaTime) = 0;
        virtual void Render(Renderer::RendererBase* pRenderer) {};
        virtual bool FrustumCull(const Math::Vector4* plane, uint32_t planeCount) const { return true; }
        virtual void OnGUI();

        virtual Math::Vector3 GetPosition() const { return mPosition; }
        virtual void SetPosition(const Math::Vector3& position) { mPosition = position; }

        virtual Math::Quaternion GetRotation() const { return mRotation; }
        virtual void SetRotation(const Math::Quaternion& rotation) { mRotation = rotation; }

        virtual Math::Vector3 GetScale() const { return mScale; }
        virtual void SetScale(const Math::Vector3& scale) { mScale = scale; }

        void SetID(uint32_t id) { mID = id; }

    protected:
        uint32_t mID = 0;
        Math::Vector3 mPosition = Math::Vector3(0.0f);
        Math::Quaternion mRotation = Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
        Math::Vector3 mScale = Math::Vector3(1.0f);
    };
} // namespace Vultana