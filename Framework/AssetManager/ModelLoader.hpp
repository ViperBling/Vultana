#pragma once

#include "Utilities/Math.hpp"
#include "AssetCooker/VAssetTypes.hpp" // FMeshCookedData / FModelCookedData (Slice 3)
#include "RHI/RHICommon.hpp"           // RHI::RHI_INVALID_RESOURCE (FMaterialTextureInfo ctor)
#include "Common/ModelConstants.hlsli"  // FMaterialTextureInfo (BuildTextureInfo returns by value)
#include "Utilities/Math.hpp"

#include <iostream>
#include <cassert>
#include <string>

#include <EASTL/string.h>

namespace tinyxml2
{
    class XMLElement;
}

struct cgltf_data;
struct cgltf_node;
struct cgltf_primitive;
struct cgltf_material;
struct cgltf_texture_view;
struct cgltf_animation;
struct cgltf_skin;

namespace RenderResources
{
    class FTexture2D;
}

namespace Scene
{
    class FAnimation;
    class FWorld;
    class FStaticMesh;
    class FSkeleton;
    struct FSkeletalMeshNode;
    struct FSkeletalMeshData;
}

namespace Assets
{
    class FMeshMaterial;
    
    class FModelLoader
    {
    public:
        FModelLoader(Scene::FWorld* pWorld);
        ~FModelLoader();

        void LoadModelSettings(tinyxml2::XMLElement* element);
        void LoadGLTF(const char* gltfFile = nullptr);
        void Load();

    private:
        // -------- Slice 3: built from FModelCookedData (no cgltf) -------- //
        void LoadStaticModel(const FModelCookedData& model);
        void LoadSkeletalModel(const FModelCookedData& model);
        
        Scene::FStaticMesh* BuildStaticMesh(const FMeshCookedData& cooked, const FAssetMaterial* assetMat, bool bFrontFaceCCW, const float3& pos, const quaternion& rot, const float3& scale);
        Scene::FSkeletalMeshNode* BuildSkeletalNode(const FAssetNode& node, int32_t nodeIndex, const FModelCookedData& model);
        Scene::FSkeletalMeshData* BuildSkeletalMeshData(const FMeshCookedData& cooked, const FAssetMaterial* assetMat, bool bSkeletalAnim, bool bFrontFaceCCW);
        FMeshMaterial* BuildMaterial(const FAssetMaterial* assetMat, bool bSkeletalAnim, bool bFrontFaceCCW);
        
        FMaterialTextureInfo BuildTextureInfo(const RenderResources::FTexture2D* texture, const FAssetTextureRef& ref);
        Scene::FAnimation* BuildAnimation(const FAssetAnimation& anim);
        Scene::FSkeleton* BuildSkeleton(const FAssetSkeleton& skeleton);

        // -------- Legacy cgltf path -------- //
        void LoadStaticMeshNode(const cgltf_data* data, cgltf_node* node, const float4x4& parentMtx);
        Scene::FStaticMesh* LoadStaticMesh(const cgltf_primitive* primitive, const eastl::string& name, bool bFrontFaceCCW);
        
        Scene::FAnimation* LoadAnimation(const cgltf_data* data, const cgltf_animation* gltfAnimation);
        Scene::FSkeleton* LoadSkeleton(const cgltf_data* data, const cgltf_skin* gltfSkin);
        Scene::FSkeletalMeshNode* LoadSkeletalMeshNode(const cgltf_data* data, cgltf_node* gltfNode);
        Scene::FSkeletalMeshData* LoadSkeletalMeshData(const cgltf_primitive* primitive, const eastl::string& name);

        FMeshMaterial* LoadMaterial(const cgltf_material* gltfMaterial);
        RenderResources::FTexture2D* LoadTexture(const cgltf_texture_view& textureView, bool srgb);

    private:
        Scene::FWorld* m_pWorld = nullptr;
        eastl::string m_File;

        float3 m_Position = float3(0.0f);
        quaternion m_Rotation = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        float3 m_Scale = float3(1.0f);
        float4x4 m_MtxWorld;
    };
}