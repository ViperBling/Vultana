#include "StaticMesh.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "Scene/Camera.hpp"

namespace Scene
{
    StaticMesh::StaticMesh(const std::string &name)
    {
        mName = name;
    }

    bool StaticMesh::Create()
    {
        return true;
    }

    void StaticMesh::Tick(float deltaTime)
    {
        float4x4 T = translation_matrix(mPosition);
        float4x4 R = rotation_matrix((RotationQuat(mRotation)));
        float4x4 S = scaling_matrix(mScale);
        mMtxWorld = mul(T, mul(R, S));

        UpdateConstants();
    }

    void StaticMesh::Render(Renderer::RendererBase *pRenderer)
    {
        Renderer::RenderBatch bassPassBatch = std::bind(&StaticMesh::RenderBassPass, this, std::placeholders::_1, std::placeholders::_2);
        pRenderer->AddForwardRenderBatch(bassPassBatch);
    }

    void StaticMesh::RenderBassPass(RHI::RHICommandList *pCmdList, const Camera *pCamera)
    {
        GPU_EVENT_DEBUG(pCmdList, mName + "::RenderBassPass");
        Draw(pCmdList, mpMaterial->GetPSO());
    }

    void StaticMesh::UpdateConstants()
    {
        mpMaterial->UpdateConstants();
        // mModelCB.MtxWorld = mMtxWorld;
        // mModelCB.MtxWorldInverse = transpose(inverse(mMtxWorld));
        mModelCB.PositionBuffer = mpPositionBuffer->GetSRV()->GetHeapIndex();
        mModelCB.TexCoordBuffer = mpTexCoordBuffer ? mpTexCoordBuffer->GetSRV()->GetHeapIndex() : RHI::RHI_INVALID_RESOURCE;
        // mModelCB.NormalBuffer = mpNormalBuffer ? mpNormalBuffer->GetSRV()->GetHeapIndex() : RHI::RHI_INVALID_RESOURCE;
        // mModelCB.TangentBuffer = mpTangentBuffer ? mpTangentBuffer->GetSRV()->GetHeapIndex() : RHI::RHI_INVALID_RESOURCE;
    }

    void StaticMesh::Draw(RHI::RHICommandList *pCmdList, RHI::RHIPipelineState *pPSO)
    {
        pCmdList->SetPipelineState(pPSO);
        pCmdList->SetGraphicsConstants(0, &mModelCB, sizeof(FModelConstants));
        pCmdList->SetGraphicsConstants(2, &mpMaterial->GetMaterialConstants(), sizeof(FModelMaterialConstants));
        pCmdList->SetIndexBuffer(mpIndexBuffer->GetBuffer(), 0, mpIndexBuffer->GetFormat());
        pCmdList->DrawIndexed(mpIndexBuffer->GetIndexCount());
    }
}