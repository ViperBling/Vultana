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

        virtual float3 GetPosition() const { return m_Position; }
        virtual void SetPosition(const float3& position) { m_Position = position; }

        virtual quaternion GetRotation() const { return m_Rotation; }
        virtual void SetRotation(const quaternion& rotation) { m_Rotation = rotation; }

        virtual float3 GetScale() const { return m_Scale; }
        virtual void SetScale(const float3& scale) { m_Scale = scale; }

        virtual const eastl::string& GetName() const { return m_Name; }
        virtual void SetName(const eastl::string& name) { m_Name = name; }

        void SetID(uint32_t id) { m_ID = id; }

    protected:
        uint32_t m_ID = 0;
        eastl::string m_Name;

        float3 m_Position = float3(0.0f);
        quaternion m_Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        float3 m_Scale = float3(1.0f);
    };
}