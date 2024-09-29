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

namespace Scene
{
    class World;
    class MeshMaterial;

    class ModelLoader
    {
    public:
        ModelLoader(World* pWorld);
        ~ModelLoader();

        void LoadGLTF(const char* gltfFile = nullptr);

    private:
        World* mpWorld = nullptr;
        std::string mFile;

        float3 mPosition = float3(0.0f);
        quaternion mRotation = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        float3 mScale = float3(1.0f);
        float4x4 mMtxWorld;
    };
}