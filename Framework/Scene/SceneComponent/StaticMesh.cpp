#include "StaticMesh.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "Scene/Camera.hpp"
#include "Utilities/GUIUtil.hpp"

namespace Scene
{
    StaticMesh::StaticMesh(const eastl::string &name)
    {
        m_Name = name;
    }

    StaticMesh::~StaticMesh()
    {
        auto resourceCache = Assets::ResourceCache::GetInstance();
        resourceCache->ReleaseSceneBuffer(m_PositionBuffer);
        resourceCache->ReleaseSceneBuffer(m_TexCoordBuffer);
        resourceCache->ReleaseSceneBuffer(m_NormalBuffer);
        resourceCache->ReleaseSceneBuffer(m_TangentBuffer);

        resourceCache->ReleaseSceneBuffer(m_MeshletBuffer);
        resourceCache->ReleaseSceneBuffer(m_MeshletVertexBuffer);
        resourceCache->ReleaseSceneBuffer(m_MeshletIndicesBuffer);

        resourceCache->ReleaseSceneBuffer(m_IndexBuffer);
    }

    bool StaticMesh::Create()
    {
        return true;
    }

    void StaticMesh::Tick(float deltaTime)
    {
        UpdateConstants();
        m_InstanceIndex = m_pRenderer->AddInstance(m_InstanceData);
    }

    void StaticMesh::Render(Renderer::RendererBase *pRenderer)
    {
        Renderer::RenderBatch& batch = pRenderer->AddBasePassBatch();

        Draw(batch, m_pMaterial->GetPSO());

        if (m_pRenderer->IsEnableMouseHitTest())
        {
            Renderer::RenderBatch& idBatch = m_pRenderer->AddObjectIDPassBatch();
            Draw(idBatch, m_pMaterial->GetIDPSO());
        }

        if (m_ID == m_pRenderer->GetMouseHitObjectID())
        {
            Renderer::RenderBatch& outlineBatch = m_pRenderer->AddOutlinePassBatch();
            Draw(outlineBatch, m_pMaterial->GetOutlinePSO());
        }
    }

    void StaticMesh::OnGUI()
    {
        IVisibleObject::OnGUI();

        m_pMaterial->OnGUI();
    }

    bool StaticMesh::FrustumCull(const float4 *plane, uint32_t planeCount) const
    {
        return ::FrustumCull(plane, planeCount, m_InstanceData.Center, m_InstanceData.Radius);
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
        m_pMaterial->UpdateConstants();

        m_InstanceData.IndexBufferAddress = m_IndexBuffer.offset;
        m_InstanceData.IndexStride = m_IndexBufferFormat == RHI::ERHIFormat::R16UI ? 2 : 4;
        m_InstanceData.TriangleCount = m_IndexCount / 3;

        m_InstanceData.MeshletCount = m_MeshletCount;
        m_InstanceData.MeshletBufferAddress = m_MeshletBuffer.offset;
        m_InstanceData.MeshletVertexBufferAddress = m_MeshletVertexBuffer.offset;
        m_InstanceData.MeshletIndexBufferAddress = m_MeshletIndicesBuffer.offset;

        m_InstanceData.PositionBufferAddress = m_PositionBuffer.offset;
        m_InstanceData.TexCoordBufferAddress = m_TexCoordBuffer.offset;
        m_InstanceData.NormalBufferAddress = m_NormalBuffer.offset;
        m_InstanceData.TangentBufferAddress = m_TangentBuffer.offset;

        m_InstanceData.bVertexAnimation = false;
        m_InstanceData.MaterialDataAddress = m_pRenderer->AllocateSceneConstantBuffer((void*)m_pMaterial->GetMaterialConstants(), sizeof(FModelMaterialConstants));
        m_InstanceData.ObjectID = m_ID;
        m_InstanceData.Scale = eastl::max(eastl::max(abs(m_Scale.x), abs(m_Scale.y)), abs(m_Scale.z));

        float4x4 T = translation_matrix(m_Position);
        float4x4 R = rotation_matrix(m_Rotation);
        float4x4 S = scaling_matrix(m_Scale);
        float4x4 mtxWorld = mul(T, mul(R, S));

        m_InstanceData.Center = mul(mtxWorld, float4(m_Center, 1.0f)).xyz();
        m_InstanceData.Radius = m_Radius * m_InstanceData.Scale;

        m_InstanceData.MtxWorld = mtxWorld;
        m_InstanceData.MtxWorldInverseTranspose = transpose(inverse(mtxWorld));
    }

    void StaticMesh::Draw(Renderer::RenderBatch& batch, RHI::RHIPipelineState *pPSO)
    {
        uint32_t rootConsts[1] = { m_InstanceIndex };

        batch.Label = m_Name.c_str();
        batch.SetPipelineState(pPSO);
        batch.SetConstantBuffer(0, rootConsts, sizeof(rootConsts));
        batch.SetIndexBuffer(m_pRenderer->GetSceneStaticBuffer(), m_IndexBuffer.offset, m_IndexBufferFormat);
        batch.DrawIndexed(m_IndexCount);
    }
}