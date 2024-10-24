#pragma once

#include "IVisibleObject.hpp"

namespace Assets
{
    class MeshMaterial;
    class ModelLoader;
}

namespace Scene
{
    class SkeletonMesh;
    class Animation;

    class Skeleton
    {
        friend class Assets::ModelLoader;
        
    public:
        Skeleton(const std::string& name);
        void Update(const SkeletonMesh* mesh);
        uint32_t GetJointMatricesAddress() const { return mJointMatricesAddress; }

    private:
        Renderer::RendererBase* mpRenderer = nullptr;
        std::string mName;
        std::vector<uint32_t> mJoints;
        std::vector<float4x4> mInverseBindMatrices;

        std::vector<float4x4> mJointMatrices;
        uint32_t mJointMatricesAddress;
    };

    struct FSkeletonMeshData
    {
        std::string Name;
        uint32_t NodeID;

        std::unique_ptr<Assets::MeshMaterial> Material;

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

        ~FSkeletonMeshData();
    };

    struct FSkeletonMeshNode
    {
        std::string Name;
        uint32_t ID;
        uint32_t Parent;
        std::vector<uint32_t> Children;
        std::vector<std::unique_ptr<FSkeletonMeshData>> Meshes;

        float3 Translation;
        quaternion Rotation;
        float3 Scale;

        float4x4 GlobalTransform;
    };

    class SkeletonMesh : public IVisibleObject
    {
        friend class Assets::ModelLoader;

    public:
        SkeletonMesh(const std::string& name);

        virtual bool Create() override;
        virtual void Tick(float deltaTime) override;
        virtual void Render(Renderer::RendererBase* pRenderer) override;
        virtual bool FrustumCull(const float4* planes, uint32_t planeCount) const override;
        virtual void OnGUI() override;

        FSkeletonMeshNode* GetNode(uint32_t nodeID) const;

    private:
        void Create(FSkeletonMeshData* mesh);

        void UpdateNodeTransform(FSkeletonMeshNode* node);
        void UpdateMeshConstants(FSkeletonMeshNode* node);

        void Draw(const FSkeletonMeshData* mesh);
        void UpdateVertexSkinning(Renderer::ComputeBatch& batch, const FSkeletonMeshData* mesh);
        void Draw(Renderer::RenderBatch& batch, const FSkeletonMeshData* mesh, RHI::RHIPipelineState* pPSO);

    private:
        Renderer::RendererBase* mpRenderer = nullptr;
        std::string mName;

        float4x4 mMtxWorld;

        std::unique_ptr<Skeleton> mpSkeleton;
        std::unique_ptr<Animation> mpAnimation;

        std::vector<std::unique_ptr<FSkeletonMeshNode>> mNodes;
        std::vector<uint32_t> mRootNodes;

        bool mbAnimated = false;
        float mRadius = 0.0f;
        float mBoundScaleFactor = 3.0f;
    };
}