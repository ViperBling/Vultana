#include "StaticMesh.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "Scene/Camera.hpp"

namespace Scene
{
    StaticMesh::StaticMesh(const std::string &name)
    {
        mName = name;
    }

    StaticMesh::~StaticMesh()
    {
        auto resourceCache = Assets::ResourceCache::GetInstance();
        resourceCache->ReleaseSceneBuffer(mPositionBuffer);
        resourceCache->ReleaseSceneBuffer(mTexCoordBuffer);
        resourceCache->ReleaseSceneBuffer(mNormalBuffer);
        resourceCache->ReleaseSceneBuffer(mTangentBuffer);
        resourceCache->ReleaseSceneBuffer(mIndexBuffer);
    }

    bool StaticMesh::Create()
    {
        return true;
    }

    void StaticMesh::Tick(float deltaTime)
    {
        UpdateConstants();
        mInstanceIndex = mpRenderer->AddInstance(mInstanceData);
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

        mInstanceData.IndexBufferAddress = mIndexBuffer.offset;
        mInstanceData.IndexStride = mIndexBufferFormat == RHI::ERHIFormat::R16UI ? 2 : 4;
        mInstanceData.TriangleCount = mIndexCount / 3;

        mInstanceData.PositionBufferAddress = mPositionBuffer.offset;
        mInstanceData.TexCoordBufferAddress = mTexCoordBuffer.offset;
        mInstanceData.NormalBufferAddress = mNormalBuffer.offset;
        mInstanceData.TangentBufferAddress = mTangentBuffer.offset;

        mInstanceData.MaterialDataAddress = mpRenderer->AllocateSceneConstantBuffer((void*)mpMaterial->GetMaterialConstants(), sizeof(FModelMaterialConstants));
        mInstanceData.ObjectID = mID;
        mInstanceData.Scale = std::max(std::max(abs(mScale.x), abs(mScale.y)), abs(mScale.z));

        float4x4 T = translation_matrix(mPosition);
        float4x4 R = rotation_matrix(mRotation);
        float4x4 S = scaling_matrix(mScale);
        float4x4 mtxWorld = mul(T, mul(R, S));

        mInstanceData.Center = mul(mtxWorld, float4(mCenter, 1.0f)).xyz();
        mInstanceData.Radius = mRadius * mInstanceData.Scale;

        mInstanceData.MtxWorld = mtxWorld;
        mInstanceData.MtxWorldInverseTranspose = transpose(inverse(mtxWorld));
    }

    void StaticMesh::Draw(RHI::RHICommandList *pCmdList, RHI::RHIPipelineState *pPSO)
    {
        uint32_t rootConsts[1] = { mInstanceIndex };

        pCmdList->SetPipelineState(pPSO);
        pCmdList->SetGraphicsConstants(0, rootConsts, sizeof(rootConsts));
        pCmdList->SetGraphicsConstants(1, &mInstanceData, sizeof(FInstanceData));
        pCmdList->SetIndexBuffer(mpRenderer->GetSceneStaticBuffer(), mIndexBuffer.offset, mIndexBufferFormat);
        pCmdList->DrawIndexed(mIndexCount);
    }
}