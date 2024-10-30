#pragma once

#include "IVisibleObject.hpp"

namespace Assets
{
    class MeshMaterial;
    class ModelLoader;
}

namespace Scene
{
    class SkeletalMesh;
    class Animation;

    class Skeleton
    {
        friend class Assets::ModelLoader;
        
    public:
        Skeleton(const eastl::string& name);
        void Update(const SkeletalMesh* mesh);
        uint32_t GetJointMatricesAddress() const { return mJointMatricesAddress; }

    private:
        Renderer::RendererBase* mpRenderer = nullptr;
        eastl::string mName;
        eastl::vector<uint32_t> mJoints;
        eastl::vector<float4x4> mInverseBindMatrices;

        eastl::vector<float4x4> mJointMatrices;
        uint32_t mJointMatricesAddress;
    };

    struct FSkeletalMeshData
    {
        eastl::string Name;
        uint32_t NodeID;

        eastl::unique_ptr<Assets::MeshMaterial> Material;

        OffsetAllocator::Allocation TexCoordBuffer;
        OffsetAllocator::Allocation JointIDBuffer;
        OffsetAllocator::Allocation JointWeightBuffer;

        OffsetAllocator::Allocation StaticPositionBuffer;
        OffsetAllocator::Allocation StaticNormalBuffer;
        OffsetAllocator::Allocation StaticTangentBuffer;

        OffsetAllocator::Allocation AnimPositionBuffer;
        OffsetAllocator::Allocation AnimNormalBuffer;
        OffsetAllocator::Allocation AnimTangentBuffer;

        // OffsetAllocator::Allocation PrevAnimPositionBuffer;

        OffsetAllocator::Allocation IndexBuffer;
        RHI::ERHIFormat IndexBufferFormat;
        uint32_t IndexCount = 0;
        uint32_t VertexCount = 0;

        FInstanceData InstanceData = {};
        uint32_t InstanceIndex = 0;

        float3 Center;
        float Radius;

        ~FSkeletalMeshData();
    };

    struct FSkeletalMeshNode
    {
        eastl::string Name;
        uint32_t ID;
        uint32_t Parent;
        eastl::vector<uint32_t> Children;
        eastl::vector<eastl::unique_ptr<FSkeletalMeshData>> Meshes;

        float3 Translation;
        quaternion Rotation;
        float3 Scale;

        float4x4 GlobalTransform;
    };

    class SkeletalMesh : public IVisibleObject
    {
        friend class Assets::ModelLoader;

    public:
        SkeletalMesh(const eastl::string& name);

        virtual bool Create() override;
        virtual void Tick(float deltaTime) override;
        virtual void Render(Renderer::RendererBase* pRenderer) override;
        virtual bool FrustumCull(const float4* planes, uint32_t planeCount) const override;
        virtual void OnGUI() override;

        FSkeletalMeshNode* GetNode(uint32_t nodeID) const;

    private:
        void Create(FSkeletalMeshData* mesh);

        void UpdateNodeTransform(FSkeletalMeshNode* node);
        void UpdateMeshConstants(FSkeletalMeshNode* node);

        void Draw(const FSkeletalMeshData* mesh);
        void UpdateVertexSkinning(Renderer::ComputeBatch& batch, const FSkeletalMeshData* mesh);
        void Draw(Renderer::RenderBatch& batch, const FSkeletalMeshData* mesh, RHI::RHIPipelineState* pPSO);

    private:
        Renderer::RendererBase* mpRenderer = nullptr;
        eastl::string mName;

        float4x4 mMtxWorld;

        eastl::unique_ptr<Skeleton> mpSkeleton;
        eastl::unique_ptr<Animation> mpAnimation;

        eastl::vector<eastl::unique_ptr<FSkeletalMeshNode>> mNodes;
        eastl::vector<uint32_t> mRootNodes;

        bool mbAnimated = false;
        bool mbResetAnim = false;
        float mRadius = 0.0f;
        float mBoundScaleFactor = 3.0f;
    };
}