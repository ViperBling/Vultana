#pragma once

#include "IVisibleObject.hpp"
#include "Renderer/RenderBatch.hpp"
#include "Utilities/Math.hpp"
#include "Common/ModelConstants.hlsli"

namespace Assets
{
    class FMeshMaterial;
    class FModelLoader;
}

namespace Scene
{
    class FCamera;

    class FStaticMesh : public IVisibleObject
    {
        friend class Assets::FModelLoader;
    public:
        FStaticMesh(const eastl::string& name);
        ~FStaticMesh();

        virtual bool Create() override;
        virtual void Tick(float deltaTime) override;
        virtual void Render(Renderer::FRendererBase* pRenderer) override;
        
        virtual void OnGUI() override;
        // virtual void SetPosition(const float3& position) override;
        // virtual void SetRotation(const quaternion& rotation) override;
        // virtual void SetScale(const float3& scale) override;
        bool FrustumCull(const float4* plane, uint32_t planeCount) const override;

        Assets::FMeshMaterial* GetMaterial() const { return m_pMaterial.get(); }

    private:
        void UpdateConstants();
        void Draw(Renderer::FRenderBatch& batch, RHI::FRHIPipelineState* pPSO);
        void Dispatch(Renderer::FRenderBatch &batch, RHI::FRHIPipelineState *pPSO);

    private:
        Renderer::FRendererBase* m_pRenderer = nullptr;
        eastl::unique_ptr<Assets::FMeshMaterial> m_pMaterial = nullptr;

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
