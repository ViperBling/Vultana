#include "SkeletalMesh.hpp"
#include "Animation.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "AssetManager/ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"
#include "Utilities/GUIUtil.hpp"

namespace Scene
{
    Skeleton::Skeleton(const eastl::string &name)
    {
        mName = name;
        mpRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
    }

    void Skeleton::Update(const SkeletalMesh *mesh)
    {
        for (size_t i = 0; i < mJoints.size(); i++)
        {
            const FSkeletalMeshNode* node = mesh->GetNode(mJoints[i]);
            mJointMatrices[i] = mul(node->GlobalTransform, mInverseBindMatrices[i]);
        }
        mJointMatricesAddress = mpRenderer->AllocateSceneConstantBuffer(mJointMatrices.data(), (uint32_t)mJointMatrices.size() * sizeof(float4x4));
    }

    FSkeletalMeshData::~FSkeletalMeshData()
    {
        Assets::ResourceCache* cache = Assets::ResourceCache::GetInstance();

        cache->ReleaseSceneBuffer(TexCoordBuffer);
        cache->ReleaseSceneBuffer(JointIDBuffer);
        cache->ReleaseSceneBuffer(JointWeightBuffer);

        cache->ReleaseSceneBuffer(StaticPositionBuffer);
        cache->ReleaseSceneBuffer(StaticNormalBuffer);
        cache->ReleaseSceneBuffer(StaticTangentBuffer);

        cache->ReleaseSceneBuffer(IndexBuffer);

        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        pRenderer->FreeSceneAnimationBuffer(AnimPositionBuffer);
        pRenderer->FreeSceneAnimationBuffer(AnimNormalBuffer);
        pRenderer->FreeSceneAnimationBuffer(AnimTangentBuffer);
        // pRenderer->FreeSceneAnimationBuffer(PrevAnimPositionBuffer);
    }

    SkeletalMesh::SkeletalMesh(const eastl::string &name)
    {
        // mpRenderer will create on load mesh.
        mName = name;
    }

    bool SkeletalMesh::Create()
    {
        for (size_t i = 0; i < mNodes.size(); i++)
        {
            for (size_t j = 0; j < mNodes[i]->Meshes.size(); j++)
            {
                auto mesh = mNodes[i]->Meshes[j].get();
                Create(mesh);
            }
        }
        return true;
    }

    void SkeletalMesh::Tick(float deltaTime)
    {
        float4x4 T = translation_matrix(mPosition);
        float4x4 R = rotation_matrix(mRotation);
        float4x4 S = scaling_matrix(mScale);
        mMtxWorld = mul(T, mul(R, S));

        if (mbResetAnim)
        {
            mpAnimation->ResetAnimation();
        }
        if (mbAnimated || (mbResetAnim && !mbAnimated))
        {
            mpAnimation->Update(this, deltaTime);
        }

        for (size_t i = 0; i < mRootNodes.size(); i++)
        {
            UpdateNodeTransform(GetNode(mRootNodes[i]));
        }
        
        if (mpSkeleton)
        {
            mpSkeleton->Update(this);
        }

        for (size_t i = 0; i < mRootNodes.size(); i++)
        {
            UpdateMeshConstants(GetNode(mRootNodes[i]));
        }
    }

    void SkeletalMesh::Render(Renderer::RendererBase *pRenderer)
    {
        for (size_t i = 0; i < mNodes.size(); i++)
        {
            for (size_t j = 0; j < mNodes[i]->Meshes.size(); j++)
            {
                const FSkeletalMeshData* mesh = mNodes[i]->Meshes[j].get();
                Draw(mesh);
            }
        }
    }

    bool SkeletalMesh::FrustumCull(const float4 *planes, uint32_t planeCount) const
    {
        return ::FrustumCull(planes, planeCount, mPosition, mRadius);
    }

    void SkeletalMesh::OnGUI()
    {
        IVisibleObject::OnGUI();
        
        GUICommand("Inspector", "SkeletalMesh", [&]()
        {
            ImGui::Checkbox("Play Animation", &mbAnimated);
            mbResetAnim = ImGui::Button("Reset Animation");
        });
    }

    FSkeletalMeshNode *SkeletalMesh::GetNode(uint32_t nodeID) const
    {
        assert(nodeID < mNodes.size());
        return mNodes[nodeID].get();
    }

    void SkeletalMesh::Create(FSkeletalMeshData *mesh)
    {
        if (mesh->Material->IsVertexSkinned())
        {
            mesh->AnimPositionBuffer = mpRenderer->AllocateSceneAnimationBuffer(sizeof(float3) * mesh->VertexCount);
            // mesh->PrevAnimPositionBuffer = mpRenderer->AllocateSceneAnimationBuffer(sizeof(float3) * mesh->VertexCount);
            if (mesh->StaticNormalBuffer.metadata != OffsetAllocator::Allocation::NO_SPACE)
            {
                mesh->AnimNormalBuffer = mpRenderer->AllocateSceneAnimationBuffer(sizeof(float3) * mesh->VertexCount);
            }
            if (mesh->StaticTangentBuffer.metadata != OffsetAllocator::Allocation::NO_SPACE)
            {
                mesh->AnimTangentBuffer = mpRenderer->AllocateSceneAnimationBuffer(sizeof(float4) * mesh->VertexCount);
            }
        }
    }

    void SkeletalMesh::UpdateNodeTransform(FSkeletalMeshNode *node)
    {
        float4x4 T = translation_matrix(node->Translation);
        float4x4 R = rotation_matrix(node->Rotation);
        float4x4 S = scaling_matrix(node->Scale);
        node->GlobalTransform = mul(T, mul(R, S));

        if (node->Parent != -1)
        {
            node->GlobalTransform = mul(GetNode(node->Parent)->GlobalTransform, node->GlobalTransform);
        }
        for (size_t i = 0; i < node->Children.size(); i++)
        {
            UpdateNodeTransform(GetNode(node->Children[i]));
        }
    }

    void SkeletalMesh::UpdateMeshConstants(FSkeletalMeshNode *node)
    {
        for (size_t i = 0; i < node->Meshes.size(); i++)
        {
            auto mesh = node->Meshes[i].get();
            // eastl::swap(mesh->PrevAnimPositionBuffer, mesh->AnimPositionBuffer);

            mesh->Material->UpdateConstants();

            mesh->InstanceData.IndexBufferAddress = mesh->IndexBuffer.offset;
            mesh->InstanceData.IndexStride = mesh->IndexBufferFormat == RHI::ERHIFormat::R32UI ? 4 : 2;
            mesh->InstanceData.TriangleCount = mesh->IndexCount / 3;

            mesh->InstanceData.TexCoordBufferAddress = mesh->TexCoordBuffer.offset;

            bool isSkinnedMesh = mesh->Material->IsVertexSkinned();
            if (isSkinnedMesh)
            {
                mesh->InstanceData.PositionBufferAddress = mesh->AnimPositionBuffer.offset;
                mesh->InstanceData.NormalBufferAddress = mesh->AnimNormalBuffer.offset;
                mesh->InstanceData.TangentBufferAddress = mesh->AnimTangentBuffer.offset;
            }
            else
            {
                mesh->InstanceData.PositionBufferAddress = mesh->StaticPositionBuffer.offset;
                mesh->InstanceData.NormalBufferAddress = mesh->StaticNormalBuffer.offset;
                mesh->InstanceData.TangentBufferAddress = mesh->StaticTangentBuffer.offset;
            }
            mesh->InstanceData.bVertexAnimation = isSkinnedMesh;
            mesh->InstanceData.MaterialDataAddress = mpRenderer->AllocateSceneConstantBuffer((void*)mesh->Material->GetMaterialConstants(), sizeof(FModelMaterialConstants));
            mesh->InstanceData.ObjectID = mID;

            auto node = GetNode(mesh->NodeID);
            float4x4 mtxNodeWorld = mul(mMtxWorld, node->GlobalTransform);

            mesh->InstanceData.Scale = max(max(abs(mScale.x), abs(mScale.y)), abs(mScale.z)) * mBoundScaleFactor;
            mesh->InstanceData.Center = mul(mMtxWorld, float4(mesh->Center, 1.0)).xyz();
            mesh->InstanceData.Radius = mesh->Radius * mesh->InstanceData.Scale;
            mRadius = max(mRadius, mesh->InstanceData.Radius);

            mesh->InstanceData.MtxWorld = isSkinnedMesh ? mMtxWorld : mtxNodeWorld;
            mesh->InstanceData.MtxWorldInverseTranspose = transpose(inverse(mesh->InstanceData.MtxWorld));

            mesh->InstanceIndex = mpRenderer->AddInstance(mesh->InstanceData);
        }
        for (size_t i = 0; i < node->Children.size(); i++)
        {
            UpdateMeshConstants(GetNode(node->Children[i]));
        }
    }

    void SkeletalMesh::Draw(const FSkeletalMeshData *mesh)
    {
        if (mesh->Material->IsVertexSkinned())
        {
            Renderer::ComputeBatch& batch = mpRenderer->AddAnimationBatch();
            UpdateVertexSkinning(batch, mesh);
        }
        Renderer::RenderBatch& batch = mpRenderer->AddBasePassBatch();
        Draw(batch, mesh, mesh->Material->GetPSO());

        if (mpRenderer->IsEnableMouseHitTest())
        {
            Renderer::RenderBatch& idBatch = mpRenderer->AddObjectIDPassBatch();
            Draw(idBatch, mesh, mesh->Material->GetIDPSO());
        }

        if (mID == mpRenderer->GetMouseHitObjectID())
        {
            Renderer::RenderBatch& outlineBatch = mpRenderer->AddOutlinePassBatch();
            Draw(outlineBatch, mesh, mesh->Material->GetOutlinePSO());
        }
    }

    void SkeletalMesh::UpdateVertexSkinning(Renderer::ComputeBatch &batch, const FSkeletalMeshData *mesh)
    {
        batch.Label = mesh->Name.c_str();
        batch.SetPipelineState(mesh->Material->GetVertexSkinningPSO());

        uint32_t cb[10] = {
            mesh->VertexCount,

            mesh->StaticPositionBuffer.offset,
            mesh->StaticNormalBuffer.offset,
            mesh->StaticTangentBuffer.offset,

            mesh->AnimPositionBuffer.offset,
            mesh->AnimNormalBuffer.offset,
            mesh->AnimTangentBuffer.offset,

            mesh->JointIDBuffer.offset,
            mesh->JointWeightBuffer.offset,
            mpSkeleton->GetJointMatricesAddress(),
        };
        batch.SetConstantBuffer(1, cb, sizeof(cb));
        batch.Dispatch((mesh->VertexCount + 63) / 64, 1, 1);
    }

    void SkeletalMesh::Draw(Renderer::RenderBatch &batch, const FSkeletalMeshData *mesh, RHI::RHIPipelineState *pPSO)
    {
        uint32_t rootConstants[1] = {
              mesh->InstanceIndex
            // , mesh->PrevAnimPositionBuffer.offset
        };

        batch.Label = mesh->Name.c_str();
        batch.SetPipelineState(pPSO);
        batch.SetConstantBuffer(0, rootConstants, sizeof(rootConstants));

        batch.SetIndexBuffer(mpRenderer->GetSceneStaticBuffer(), mesh->IndexBuffer.offset, mesh->IndexBufferFormat);
        batch.DrawIndexed(mesh->IndexCount);
    }
}