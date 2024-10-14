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
        StaticMesh(const std::string& name);
        ~StaticMesh();

        virtual bool Create() override;
        virtual void Tick(float deltaTime) override;
        virtual void Render(Renderer::RendererBase* pRenderer) override;

    private:
        void RenderBassPass(RHI::RHICommandList* pCmdList, const Camera* pCamera);

        void UpdateConstants();
        void Draw(RHI::RHICommandList* pCmdList, RHI::RHIPipelineState* pPSO);
    
    private:
        Renderer::RendererBase* mpRenderer = nullptr;
        std::string mName;
        std::unique_ptr<Assets::MeshMaterial> mpMaterial = nullptr;

        // std::unique_ptr<RenderResources::IndexBuffer> mpIndexBuffer = nullptr;
        // std::unique_ptr<RenderResources::StructuredBuffer> mpPositionBuffer = nullptr;
        // std::unique_ptr<RenderResources::StructuredBuffer> mpTexCoordBuffer = nullptr;
        // std::unique_ptr<RenderResources::StructuredBuffer> mpNormalBuffer = nullptr;
        // std::unique_ptr<RenderResources::StructuredBuffer> mpTangentBuffer = nullptr;

        OffsetAllocator::Allocation mIndexBuffer;
        OffsetAllocator::Allocation mPositionBuffer;
        OffsetAllocator::Allocation mTexCoordBuffer;
        OffsetAllocator::Allocation mNormalBuffer;
        OffsetAllocator::Allocation mTangentBuffer;

        RHI::ERHIFormat mIndexBufferFormat;
        uint32_t mIndexCount = 0;
        uint32_t mVertexCount = 0;

        FInstanceData mInstanceData = {};
        uint32_t mInstanceIndex = 0;

        float3 mCenter = float3(0.0f);
        float mRadius = 0.0f;
    };
} // namespace Scene
