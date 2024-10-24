#pragma once

#include <iostream>
#include <cassert>
#include <string>

#include "Utilities/Math.hpp"

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
    class Texture2D;
}

namespace Scene
{
    class Animation;
    class World;
    class StaticMesh;
    class Skeleton;
    struct FSkeletalMeshNode;
    struct FSkeletalMeshData;
}

namespace Assets
{
    class MeshMaterial;
    
    class ModelLoader
    {
    public:
        ModelLoader(Scene::World* pWorld);
        ~ModelLoader();

        void LoadModelSettings(tinyxml2::XMLElement* element);
        void LoadGLTF(const char* gltfFile = nullptr);

    private:
        void LoadStaticMeshNode(const cgltf_data* data, cgltf_node* node, const float4x4& parentMtx);
        Scene::StaticMesh* LoadStaticMesh(const cgltf_primitive* primitive, const std::string& name, bool bFrontFaceCCW);
        
        Scene::Animation* LoadAnimation(const cgltf_data* data, const cgltf_animation* gltfAnimation);
        Scene::Skeleton* LoadSkeleton(const cgltf_data* data, const cgltf_skin* gltfSkin);
        Scene::FSkeletalMeshData* LoadSkeletalMeshData(const cgltf_primitive* primitive, const std::string& name);
        Scene::FSkeletalMeshNode* LoadSkeletalMeshNode(const cgltf_data* data, cgltf_node* gltfNode);

        MeshMaterial* LoadMaterial(const cgltf_material* gltfMaterial);
        RenderResources::Texture2D* LoadTexture(const cgltf_texture_view& textureView, bool srgb);

    private:
        Scene::World* mpWorld = nullptr;
        std::string mFile;

        float3 mPosition = float3(0.0f);
        quaternion mRotation = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        float3 mScale = float3(1.0f);
        float4x4 mMtxWorld;
    };
}