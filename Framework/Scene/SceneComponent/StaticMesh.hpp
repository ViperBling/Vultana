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

        Assets::MeshMaterial* GetMaterial() const { return mpMaterial.get(); }

    private:
        void UpdateConstants();
        void Draw(Renderer::RenderBatch& batch, RHI::RHIPipelineState* pPSO);
    
    private:
        Renderer::RendererBase* mpRenderer = nullptr;
        eastl::unique_ptr<Assets::MeshMaterial> mpMaterial = nullptr;

        OffsetAllocator::Allocation mPositionBuffer;
        OffsetAllocator::Allocation mTexCoordBuffer;
        OffsetAllocator::Allocation mNormalBuffer;
        OffsetAllocator::Allocation mTangentBuffer;

        OffsetAllocator::Allocation mMeshletBuffer;
        OffsetAllocator::Allocation mMeshletIndicesBuffer;
        OffsetAllocator::Allocation mMeshletVertexBuffer;
        uint32_t mMeshletCount = 0;

        OffsetAllocator::Allocation mIndexBuffer;
        RHI::ERHIFormat mIndexBufferFormat;
        uint32_t mIndexCount = 0;
        uint32_t mVertexCount = 0;

        FInstanceData mInstanceData = {};
        uint32_t mInstanceIndex = 0;

        float3 mCenter = float3(0.0f);
        float mRadius = 0.0f;
    };
} // namespace Scene
