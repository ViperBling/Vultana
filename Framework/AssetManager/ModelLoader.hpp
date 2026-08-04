#pragma once

#include <iostream>
#include <cassert>
#include <string>

#include "Utilities/Math.hpp"

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

    private:
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