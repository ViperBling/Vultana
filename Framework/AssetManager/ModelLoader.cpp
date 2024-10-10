#include "ModelLoader.hpp"
#include "Scene/SceneComponent/StaticMesh.hpp"
#include "MeshMaterial.hpp"
#include "Core/VultanaEngine.hpp"

#include "Utilities/Log.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include <tinyxml2/tinyxml2.h>

#include <cassert>

inline void GetTransform(cgltf_node* node, float4x4& matrix)
{
    float3 translation;
    float4 rotation;
    float3 scale;

    if (node->has_matrix)
    {
        float4x4 matrix = float4x4(node->matrix);
        Decompose(matrix, translation, rotation, scale);
    }
    else
    {
        translation = float3(node->translation);
        rotation = float4(node->rotation);
        scale = float3(node->scale);
    }

    // RH to LH
    translation.z *= -1;
    rotation.z *= -1;
    rotation.w *= -1;

    float4x4 T = translation_matrix(translation);
    float4x4 R = rotation_matrix(rotation);
    float4x4 S = scaling_matrix(scale);

    matrix = mul(T, mul(R, S));
}

inline bool IsFrontFaceCCW(const cgltf_node* node)
{
    cgltf_float m[16];
    cgltf_node_transform_world(node, m);

    float4x4 mtx(m);

    return determinant(mtx) > 0.0f;
}

inline uint32_t GetMeshIndex(const cgltf_data* data, const cgltf_mesh* mesh)
{
    for (cgltf_size i = 0; i < data->meshes_count; i++)
    {
        if (&data->meshes[i] == mesh)
        {
            return static_cast<uint32_t>(i);
        }
    }
    assert(false);
    return 0;
}

namespace Assets
{
    ModelLoader::ModelLoader(Scene::World *pWorld)
    {
        mpWorld = pWorld;
        // TODO : File path
        mFile = "";

        float3 position = { 0.0f, 0.0f, 0.0f };
        float3 rotation = { 0.0f, 0.0f, 0.0f };
        float3 scale = { 1.0f, 1.0f, 1.0f };

        float4x4 T = translation_matrix(position);
        float4x4 R = rotation_matrix(RotationQuat(rotation));
        float4x4 S = scaling_matrix(scale);

        mMtxWorld = mul(T, mul(R, S));
    }

    ModelLoader::~ModelLoader()
    {
    }

    void ModelLoader::LoadGLTF(const char *gltfFile)
    {
        std::string file = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath() + (gltfFile ? gltfFile : mFile);

        cgltf_options options = {};
        cgltf_data* data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, file.c_str(), &data);
        if (result != cgltf_result_success)
        {
            VTNA_LOG_ERROR("[ModelLoader::LoadGLTF] failed to parse gltf file: {}", file);
            return;
        }

        cgltf_load_buffers(&options, data, file.c_str());

        for (cgltf_size i = 0; i < data->scenes_count; i++)
        {
            for (cgltf_size node = 0; node < data->scenes[i].nodes_count; node++)
            {
                LoadNode(data, data->scenes[i].nodes[node], mMtxWorld);
            }
        }
        cgltf_free(data);
    }

    void ModelLoader::LoadNode(const cgltf_data* data, cgltf_node *node, const float4x4 &parentMtx)
    {
        float4x4 mtxLocalToParent;
        GetTransform(node, mtxLocalToParent);

        float4x4 mtxWorld = mul(parentMtx, mtxLocalToParent);

        if (node->mesh)
        {
            float3 position;
            float3 rotation;
            float3 scale;
            Decompose(mtxWorld, position, rotation, scale);

            uint32_t meshIdx = GetMeshIndex(data, node->mesh);
            bool bFrontFaceCCW = IsFrontFaceCCW(node);

            for (cgltf_size i = 0; i < node->mesh->primitives_count; i++)
            {
                Scene::StaticMesh* mesh = LoadStaticMesh(&node->mesh->primitives[i], node->name ? node->name : "");
                mesh->SetPosition(position);
                mesh->SetRotation(rotation);
                mesh->SetScale(scale);
            }
        }
        for (cgltf_size i = 0; i < node->children_count; i++)
        {
            LoadNode(data, node->children[i], mtxWorld);
        }
    }

    Scene::StaticMesh *ModelLoader::LoadStaticMesh(cgltf_primitive *primitive, const std::string &name)
    {
        return nullptr;
    }

    MeshMaterial *ModelLoader::LoadMaterial(cgltf_material *material)
    {
        return nullptr;
    }

    RenderResources::Texture2D *ModelLoader::LoadTexture(const cgltf_texture_view& textureView, bool srgb)
    {
        if (textureView.texture == nullptr || textureView.texture->image->uri == nullptr) return nullptr;

        size_t lastSlash = mFile.find_last_of('/');
        std::string texturePath = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath() + mFile.substr(0, lastSlash + 1);
        Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto texture = pRenderer->CreateTexture2D(texturePath + textureView.texture->image->uri, srgb);
        return texture;
    }
}