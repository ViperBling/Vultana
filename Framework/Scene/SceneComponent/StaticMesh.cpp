#include "StaticMesh.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "Scene/Camera.hpp"
#include "Utilities/GUIUtil.hpp"

namespace Scene
{
    StaticMesh::StaticMesh(const eastl::string &name)
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
        Renderer::RenderBatch& batch = pRenderer->AddBasePassBatch();

        Draw(batch, mpMaterial->GetPSO());

        if (mpRenderer->IsEnableMouseHitTest())
        {
            Renderer::RenderBatch& idBatch = mpRenderer->AddObjectIDPassBatch();
            Draw(idBatch, mpMaterial->GetIDPSO());
        }

        if (mID == mpRenderer->GetMouseHitObjectID())
        {
            Renderer::RenderBatch& outlineBatch = mpRenderer->AddOutlinePassBatch();
            Draw(outlineBatch, mpMaterial->GetOutlinePSO());
        }
    }

    void StaticMesh::OnGUI()
    {
        IVisibleObject::OnGUI();

        mpMaterial->OnGUI();
    }

    // void StaticMesh::SetPosition(const float3 &position)
    // {
    //     IVisibleObject::SetPosition(position);
    // }

    // void StaticMesh::SetRotation(const quaternion &rotation)
    // {
    //     IVisibleObject::SetRotation(rotation);
    // }

    // void StaticMesh::SetScale(const float3 &scale)
    // {
    //     IVisibleObject::SetScale(scale);
    // }

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

        mInstanceData.bVertexAnimation = false;
        mInstanceData.MaterialDataAddress = mpRenderer->AllocateSceneConstantBuffer((void*)mpMaterial->GetMaterialConstants(), sizeof(FModelMaterialConstants));
        mInstanceData.ObjectID = mID;
        mInstanceData.Scale = eastl::max(eastl::max(abs(mScale.x), abs(mScale.y)), abs(mScale.z));

        float4x4 T = translation_matrix(mPosition);
        float4x4 R = rotation_matrix(mRotation);
        float4x4 S = scaling_matrix(mScale);
        float4x4 mtxWorld = mul(T, mul(R, S));

        mInstanceData.Center = mul(mtxWorld, float4(mCenter, 1.0f)).xyz();
        mInstanceData.Radius = mRadius * mInstanceData.Scale;

        mInstanceData.MtxWorld = mtxWorld;
        mInstanceData.MtxWorldInverseTranspose = transpose(inverse(mtxWorld));
    }

    void StaticMesh::Draw(Renderer::RenderBatch& batch, RHI::RHIPipelineState *pPSO)
    {
        uint32_t rootConsts[1] = { mInstanceIndex };

        batch.Label = mName.c_str();
        batch.SetPipelineState(pPSO);
        batch.SetConstantBuffer(0, rootConsts, sizeof(rootConsts));
        batch.SetIndexBuffer(mpRenderer->GetSceneStaticBuffer(), mIndexBuffer.offset, mIndexBufferFormat);
        batch.DrawIndexed(mIndexCount);
    }
}