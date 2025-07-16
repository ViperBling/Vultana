#pragma once

#include "IVisibleObject.hpp"
#include "Renderer/RenderBatch.hpp"
#include "Utilities/Math.hpp"
#include "Common/ModelConstants.hlsli"

namespace Assets
{
    class MeshMaterial;
    class ModelLoader;
}

namespace Scene
{
    class Camera;

    class StaticMesh : public IVisibleObject
    {
        friend class Assets::ModelLoader;
    public:
        StaticMesh(const eastl::string& name);
        ~StaticMesh();

        virtual bool Create() override;
        virtual void Tick(float deltaTime) override;
        virtual void Render(Renderer::RendererBase* pRenderer) override;
        
        virtual void OnGUI() override;
        // virtual void SetPosition(const float3& position) override;
        // virtual void SetRotation(const quaternion& rotation) override;
        // virtual void SetScale(const float3& scale) override;
        bool FrustumCull(const float4* plane, uint32_t planeCount) const override;

        Assets::MeshMaterial* GetMaterial() const { return m_pMaterial.get(); }

    private:
        void UpdateConstants();
        void Draw(Renderer::RenderBatch& batch, RHI::RHIPipelineState* pPSO);
    
    private:
        Renderer::RendererBase* m_pRenderer = nullptr;
        eastl::unique_ptr<Assets::MeshMaterial> m_pMaterial = nullptr;

        OffsetAllocator::Allocation m_PositionBuffer;
        OffsetAllocator::Allocation m_TexCoordBuffer;
        OffsetAllocator::Allocation m_NormalBuffer;
        OffsetAllocator::Allocation m_TangentBuffer;

        OffsetAllocator::Allocation m_MeshletBuffer;
        OffsetAllocator::Allocation m_MeshletIndicesBuffer;
        OffsetAllocator::Allocation m_MeshletVertexBuffer;
        uint32_t m_MeshletCount = 0;

        OffsetAllocator::Allocation m_IndexBuffer;
        RHI::ERHIFormat m_IndexBufferFormat;
        uint32_t m_IndexCount = 0;
        uint32_t m_VertexCount = 0;

        FInstanceData m_InstanceData = {};
        uint32_t m_InstanceIndex = 0;

        float3 m_Center = float3(0.0f);
        float m_Radius = 0.0f;
    };
} // namespace Scene
