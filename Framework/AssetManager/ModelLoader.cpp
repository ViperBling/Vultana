#include "ModelLoader.hpp"
#include "Scene/SceneComponent/StaticMesh.hpp"
#include "MeshMaterial.hpp"
#include "ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"

#include "Utilities/Log.hpp"
#include "Utilities/Memory.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include <tinyxml2/tinyxml2.h>
#include <meshoptimizer.h>

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
        mFile = "Models/BusterDrone/busterDrone.gltf";

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
            float4 rotation;
            float3 scale;
            Decompose(mtxWorld, position, rotation, scale);

            uint32_t meshIdx = GetMeshIndex(data, node->mesh);
            bool bFrontFaceCCW = IsFrontFaceCCW(node);

            for (cgltf_size i = 0; i < node->mesh->primitives_count; i++)
            {
                std::string name = fmt::format("Mesh_{}_{} {}", meshIdx, i, (node->name ? node->name : "")).c_str();
                Scene::StaticMesh* mesh = LoadStaticMesh(&node->mesh->primitives[i], name, bFrontFaceCCW);
                mesh->mpMaterial->mbFrontFaceCCW = bFrontFaceCCW;
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

    RenderResources::IndexBuffer* LoadIndexBuffer(const cgltf_accessor* accessor, const std::string& name)
    {
        assert(accessor->component_type == cgltf_component_type_r_16u || accessor->component_type == cgltf_component_type_r_32u);
        uint32_t stride = (uint32_t)accessor->stride;
        uint32_t indexCount = (uint32_t)accessor->count;
        void* data = (char*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;

        auto renderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto indexBuffer = renderer->CreateIndexBuffer(data, stride, indexCount, name);
        return indexBuffer;
    }

    RenderResources::StructuredBuffer* LoadVertexBuffer(const cgltf_accessor* accessor, const std::string& name, bool bConvertToLH)
    {
        assert(accessor->component_type == cgltf_component_type_r_32f || accessor->component_type == cgltf_component_type_r_16u);
        uint32_t stride = (uint32_t)accessor->stride;
        uint32_t size = stride * (uint32_t)accessor->count;
        void* data = (char*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;

        if (bConvertToLH)
        {
            for (uint32_t i = 0; i < (uint32_t)accessor->count; i++)
            {
                float3* v = (float3*)data + i;
                v->z = -v->z;
            }
        }
        auto renderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto vertexBuffer = renderer->CreateStructuredBuffer(data, stride, (uint32_t)accessor->count, name);
        return vertexBuffer;
    }

    meshopt_Stream LoadBufferStream(const cgltf_accessor* accessor, bool convertToLH, size_t& count)
    {
        uint32_t stride = (uint32_t)accessor->stride;

        meshopt_Stream stream = {};
        stream.data = VTNA_ALLOC(stride * accessor->count);
        stream.size = stride;
        stream.stride = stride;

        void* data = (char*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;
        memcpy((void*)stream.data, data, stride * accessor->count);

        //convert right-hand to left-hand
        if (convertToLH)
        {
            assert(stride >= sizeof(float3));

            for (uint32_t i = 0; i < (uint32_t)accessor->count; ++i)
            {
                float3* v = (float3*)stream.data + i;
                v->z = -v->z;
            }
        }
        count = accessor->count;

        return stream;
    }

    Scene::StaticMesh *ModelLoader::LoadStaticMesh(const cgltf_primitive *primitive, const std::string &name, bool bFrontFaceCCW)
    {
        Scene::StaticMesh* mesh = new Scene::StaticMesh(mFile + " " + name);
        mesh->mpMaterial.reset(LoadMaterial(primitive->material));

        for (cgltf_size i = 0; i < primitive->attributes_count; i++)
        {
            switch (primitive->attributes[i].type)
            {
            case cgltf_attribute_type_position:
                mesh->mpPositionBuffer.reset(LoadVertexBuffer(primitive->attributes[i].data, "model(" + mFile + "_" + name + ")" + "_Position", bFrontFaceCCW));
                break;
            case cgltf_attribute_type_texcoord:
                if (primitive->attributes[i].index == 0)
                {
                    mesh->mpTexCoordBuffer.reset(LoadVertexBuffer(primitive->attributes[i].data, "model(" + mFile + "_" + name + ")" + "_TexCoord", bFrontFaceCCW));
                }
                break;
            case cgltf_attribute_type_normal:
                mesh->mpNormalBuffer.reset(LoadVertexBuffer(primitive->attributes[i].data, "model(" + mFile + "_" + name + ")" + "_Normal", bFrontFaceCCW));
                break;
            case cgltf_attribute_type_tangent:
                mesh->mpTangentBuffer.reset(LoadVertexBuffer(primitive->attributes[i].data, "model(" + mFile + "_" + name + ")" + "_Tangent", bFrontFaceCCW));
                break;
            default:
                break;
            }
        }
        mpWorld->AddObject(mesh);
        return mesh;
    }

    inline FMaterialTextureInfo LoadTextureInfo(const RenderResources::Texture2D* texture, const cgltf_texture_view& textureView)
    {
        FMaterialTextureInfo info;
        if (texture)
        {
            info.Index = texture->GetSRV()->GetHeapIndex();
            info.Width = texture->GetTexture()->GetDesc().Width;
            info.Height = texture->GetTexture()->GetDesc().Height;
            if (textureView.has_transform)
            {
                info.IsTransform = true;
                info.Offset = float2(textureView.transform.offset);
                info.Scale = float2(textureView.transform.scale);
                info.Rotation = textureView.transform.rotation;
            }
        }
        return info;
    }

    MeshMaterial *ModelLoader::LoadMaterial(const cgltf_material *gltfMaterial)
    {
        MeshMaterial* material = new MeshMaterial;
        if (gltfMaterial == nullptr)
        {
            return material;
        }
        material->mName = gltfMaterial->name ? gltfMaterial->name : "";

        if (gltfMaterial->has_pbr_metallic_roughness)
        {
            material->mWorkFlow = MaterialWorkFlow::PBRMetallicRoughness;
            material->mpAlbedoTexture = LoadTexture(gltfMaterial->pbr_metallic_roughness.base_color_texture, true);
            material->mMaterialCB.AlbedoTexture = LoadTextureInfo(material->mpAlbedoTexture, gltfMaterial->pbr_metallic_roughness.base_color_texture);
            material->mpMetallicRoughTexture = LoadTexture(gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture, false);
            material->mMaterialCB.MetallicRoughnessTexture = LoadTextureInfo(material->mpMetallicRoughTexture, gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture);
            material->mAlbedoColor = float3(gltfMaterial->pbr_metallic_roughness.base_color_factor);
            material->mMetallic = gltfMaterial->pbr_metallic_roughness.metallic_factor;
            material->mRoughness = gltfMaterial->pbr_metallic_roughness.roughness_factor;
        }
        else if (gltfMaterial->has_pbr_specular_glossiness)
        {
            material->mWorkFlow = MaterialWorkFlow::PBRSpecularGlossiness;
            material->mpDiffuseTexture = LoadTexture(gltfMaterial->pbr_specular_glossiness.diffuse_texture, true);
            material->mMaterialCB.DiffuseTexture = LoadTextureInfo(material->mpDiffuseTexture, gltfMaterial->pbr_specular_glossiness.diffuse_texture);
            material->mpSpecularGlossinessTexture = LoadTexture(gltfMaterial->pbr_specular_glossiness.specular_glossiness_texture, false);
            material->mMaterialCB.SpecularGlossinessTexture = LoadTextureInfo(material->mpSpecularGlossinessTexture, gltfMaterial->pbr_specular_glossiness.specular_glossiness_texture);
            material->mDiffuseColor = float3(gltfMaterial->pbr_specular_glossiness.diffuse_factor);
            material->mSpecularColor = float3(gltfMaterial->pbr_specular_glossiness.specular_factor);
            material->mGlossiness = gltfMaterial->pbr_specular_glossiness.glossiness_factor;
        }
        material->mpNormalTexture = LoadTexture(gltfMaterial->normal_texture, false);
        material->mMaterialCB.NormalTexture = LoadTextureInfo(material->mpNormalTexture, gltfMaterial->normal_texture);
        material->mpEmissiveTexture = LoadTexture(gltfMaterial->emissive_texture, true);
        material->mMaterialCB.EmissiveTexture = LoadTextureInfo(material->mpEmissiveTexture, gltfMaterial->emissive_texture);
        material->mpAOTexture = LoadTexture(gltfMaterial->occlusion_texture, false);
        material->mMaterialCB.AmbientOcclusionTexture = LoadTextureInfo(material->mpAOTexture, gltfMaterial->occlusion_texture);

        material->mEmissiveColor = float3(gltfMaterial->emissive_factor);
        material->mAlphaCutout = gltfMaterial->alpha_cutoff;
        material->mbAlphaTest = gltfMaterial->alpha_mode == cgltf_alpha_mode_mask;
        material->mbAlphaBlend = gltfMaterial->alpha_mode == cgltf_alpha_mode_blend;
        material->mbFrontFaceCCW = gltfMaterial->double_sided;

        return material;
    }

    RenderResources::Texture2D *ModelLoader::LoadTexture(const cgltf_texture_view& textureView, bool srgb)
    {
        if (textureView.texture == nullptr || textureView.texture->image->uri == nullptr) return nullptr;

        size_t lastSlash = mFile.find_last_of('/');
        std::string texturePath = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath() + mFile.substr(0, lastSlash + 1);
        Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto texture = ResourceCache::GetInstance()->GetTexture2D(texturePath + textureView.texture->image->uri, srgb);
        return texture;
    }
}