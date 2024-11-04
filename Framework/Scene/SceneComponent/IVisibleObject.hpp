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
        virtual bool FrustumCull(const float4* plane, uint32_t planeCount) const { return true; }
        virtual void OnGUI();

        virtual float3 GetPosition() const { return mPosition; }
        virtual void SetPosition(const float3& position) { mPosition = position; }

        virtual quaternion GetRotation() const { return mRotation; }
        virtual void SetRotation(const quaternion& rotation) { mRotation = rotation; }

        virtual float3 GetScale() const { return mScale; }
        virtual void SetScale(const float3& scale) { mScale = scale; }

        virtual const eastl::string& GetName() const { return mName; }
        virtual void SetName(const eastl::string& name) { mName = name; }

        void SetID(uint32_t id) { mID = id; }

    protected:
        uint32_t mID = 0;
        eastl::string mName;

        float3 mPosition = float3(0.0f);
        quaternion mRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        float3 mScale = float3(1.0f);
    };
}