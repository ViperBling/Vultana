#pragma once

#include "Utilities/Math.hpp"
#include "Scene/ECS/AssetRegistry.hpp"
#include "Common/GPUScene.hlsli"

#include <cstdint>

namespace Scene::ECS
{
    // Render batch input: MaterialHandle for bridge layer to parse PSO (Core zero RHI),
    // IndexBuffer* for ID/Outline non-meshlet path usage (aligned with legacy StaticMesh::Draw path).
    struct FRenderBatchInput
    {
        uint32_t InstanceIndex  = 0;
        uint32_t MeshletCount = 0;
        float3   Center         = float3(0.0f);
        float    Radius         = 0.0f;
        FMaterialResourceHandle MaterialHandle;
        uint32_t IndexBufferAddress = 0;
        uint32_t IndexStride    = 2;
        uint32_t IndexCount     = 0;
        const char* Label       = "";
    };

    struct FAnimationBatchInput
    {
        FMaterialResourceHandle MaterialHandle;
        uint32_t VertexCount                 = 0;
        uint32_t StaticPositionBufferAddress = 0;
        uint32_t StaticNormalBufferAddress   = 0;
        uint32_t StaticTangentBufferAddress  = 0;
        uint32_t AnimPositionBufferAddress   = 0;
        uint32_t AnimNormalBufferAddress     = 0;
        uint32_t AnimTangentBufferAddress    = 0;
        uint32_t JointIDBufferAddress        = 0;
        uint32_t JointWeightBufferAddress    = 0;
        uint32_t JointMatricesAddress        = 0;
        const char* Label = "";
    };

    class ISceneRenderSink
    {
    public:
        virtual ~ISceneRenderSink() = default;
        virtual uint32_t AddInstance(const FInstanceData &instanceData) = 0;
        virtual void AddBasePassBatch(const FRenderBatchInput &input) = 0;
        virtual void AddObjectIDBatch(const FRenderBatchInput &input) = 0;
        virtual void AddOutlineBatch(const FRenderBatchInput &input) = 0;
        virtual uint32_t AllocateMaterialConstants(const void *data, uint32_t size) = 0;
        virtual uint32_t GetSelectedPickID() const = 0;

        virtual bool IsEnableMouseHitTest() const = 0;
        virtual void AddAnimationBatch(const FAnimationBatchInput &input) = 0;
    };
}