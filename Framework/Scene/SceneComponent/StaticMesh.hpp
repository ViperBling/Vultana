#pragma once

#include "IVisibleObject.hpp"
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

        virtual bool Create() override;
        virtual void Tick(float deltaTime) override;
        virtual void Render(Renderer::RendererBase* pRenderer) override;

    private:
        void RenderBassPass(RHI::RHICommandList* pCmdList, const Camera* pCamera);

        void UpdateConstants();
        void Draw(RHI::RHICommandList* pCmdList, RHI::RHIPipelineState* pPSO);
    
    private:
        std::string mName;
        std::unique_ptr<Assets::MeshMaterial> mpMaterial = nullptr;

        std::unique_ptr<RenderResources::IndexBuffer> mpIndexBuffer = nullptr;
        std::unique_ptr<RenderResources::StructuredBuffer> mpPositionBuffer = nullptr;
        std::unique_ptr<RenderResources::StructuredBuffer> mpTexCoordBuffer = nullptr;
        std::unique_ptr<RenderResources::StructuredBuffer> mpNormalBuffer = nullptr;
        std::unique_ptr<RenderResources::StructuredBuffer> mpTangentBuffer = nullptr;

        FModelConstants mModelCB = {};
        
        float4x4 mMtxWorld;
    };
} // namespace Scene
