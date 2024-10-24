#include "ModelLoader.hpp"
#include "Scene/SceneComponent/Animation.hpp"
#include "Scene/SceneComponent/StaticMesh.hpp"
#include "Scene/SceneComponent/SkeletonMesh.hpp"
#include "MeshMaterial.hpp"
#include "ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"

#include "Utilities/Log.hpp"
#include "Utilities/Memory.hpp"
#include "Utilities/String.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include <tinyxml2/tinyxml2.h>
#include <meshoptimizer.h>

#include <cassert>

inline float3 strToFloat3(const std::string& str)
{
    std::vector<float> v;
    v.reserve(3);
    StringUtils::StringToFloatArray(str, v);
    return float3(v[0], v[1], v[2]);
}

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

inline uint32_t GetNodeIndex(const cgltf_data* data, const cgltf_node* node)
{
    for (cgltf_size i = 0; i < data->nodes_count; i++)
    {
        if (&data->nodes[i] == node)
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

        float4x4 T = translation_matrix(mPosition);
        float4x4 R = rotation_matrix(mRotation);
        float4x4 S = scaling_matrix(mScale);

        mMtxWorld = mul(T, mul(R, S));
    }

    ModelLoader::~ModelLoader()
    {
    }

    void ModelLoader::LoadModelSettings(tinyxml2::XMLElement *element)
    {
        mFile = element->FindAttribute("File")->Value();

        const tinyxml2::XMLAttribute* positionAttr = element->FindAttribute("Position");
        if (positionAttr)
        {
            mPosition = strToFloat3(positionAttr->Value());
        }
        const tinyxml2::XMLAttribute* rotationAttr = element->FindAttribute("Rotation");
        if (rotationAttr)
        {
            mRotation = RotationQuat(strToFloat3(rotationAttr->Value()));
        }
        const tinyxml2::XMLAttribute* scaleAttr = element->FindAttribute("Scale");
        if (scaleAttr)
        {
            mScale = strToFloat3(scaleAttr->Value());
        }

        float4x4 T = translation_matrix(mPosition);
        float4x4 R = rotation_matrix(mRotation);
        float4x4 S = scaling_matrix(mScale);
        mMtxWorld = mul(T, mul(R, S));
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

        if (data->animations_count > 0)
        {
            Scene::SkeletonMesh* mesh = new Scene::SkeletonMesh(mFile);
            mesh->mpRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
            mesh->mpAnimation.reset(LoadAnimation(data, &data->animations[0]));
            mesh->mpSkeleton.reset(LoadSkeleton(data, &data->skins[0]));

            for (cgltf_size i = 0; i < data->nodes_count; i++)
            {
                mesh->mNodes.emplace_back(LoadSkeletalMeshNode(data, &data->nodes[i]));
            }
            for (cgltf_size i = 0; i < data->scene->nodes_count; i++)
            {
                mesh->mRootNodes.push_back(GetNodeIndex(data, data->scene->nodes[i]));
            }

            mesh->SetPosition(mPosition);
            mesh->SetRotation(mRotation);
            mesh->SetScale(mScale);
            mesh->Create();
            mpWorld->AddObject(mesh);
        }
        else
        {
            for (cgltf_size i = 0; i < data->scenes_count; i++)
            {
                for (cgltf_size node = 0; node < data->scenes[i].nodes_count; node++)
                {
                    LoadStaticMeshNode(data, data->scenes[i].nodes[node], mMtxWorld);
                }
            }
        }
        
        cgltf_free(data);
    }

    void ModelLoader::LoadStaticMeshNode(const cgltf_data* data, cgltf_node *node, const float4x4 &parentMtx)
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
                std::string name = fmt::format("Mesh_{}_{} : {}", meshIdx, i, (node->name ? node->name : "")).c_str();
                Scene::StaticMesh* mesh = LoadStaticMesh(&node->mesh->primitives[i], name, bFrontFaceCCW);
                mesh->mpMaterial->mbFrontFaceCCW = bFrontFaceCCW;
                mesh->SetPosition(position);
                mesh->SetRotation(rotation);
                mesh->SetScale(scale);
            }
        }
        for (cgltf_size i = 0; i < node->children_count; i++)
        {
            LoadStaticMeshNode(data, node->children[i], mtxWorld);
        }
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

        size_t indexCount;
        meshopt_Stream indices = LoadBufferStream(primitive->indices, false, indexCount);

        size_t vertexCount;
        std::vector<meshopt_Stream> vertexStreams;
        std::vector<cgltf_attribute_type> vertexTypes;

        for (cgltf_size i = 0; i < primitive->attributes_count; i++)
        {
            switch (primitive->attributes[i].type)
            {
            case cgltf_attribute_type_position:
                vertexStreams.push_back(LoadBufferStream(primitive->attributes[i].data, true, vertexCount));
                vertexTypes.push_back(primitive->attributes[i].type);
                {
                    float3 min = float3(primitive->attributes[i].data->min);
                    min.z = -min.z;
                    float3 max = float3(primitive->attributes[i].data->max);
                    max.z = -max.z;
                    float3 center = (min + max) * 0.5f;
                    float radius = length(max - min) * 0.5f;
                    mesh->mCenter = center;
                    mesh->mRadius = radius;
                }
                break;
            case cgltf_attribute_type_texcoord:
                if (primitive->attributes[i].index == 0)
                {
                    vertexStreams.push_back(LoadBufferStream(primitive->attributes[i].data, false, vertexCount));
                    vertexTypes.push_back(primitive->attributes[i].type);
                }
                break;
            case cgltf_attribute_type_normal:
                vertexStreams.push_back(LoadBufferStream(primitive->attributes[i].data, true, vertexCount));
                vertexTypes.push_back(primitive->attributes[i].type);
                break;
            case cgltf_attribute_type_tangent:
                vertexStreams.push_back(LoadBufferStream(primitive->attributes[i].data, true, vertexCount));
                vertexTypes.push_back(primitive->attributes[i].type);
                break;
            default:
                break;
            }
        }
        std::vector<unsigned int> remap(indexCount);

        void* remappedIndices = VTNA_ALLOC(indices.stride * indexCount);
        std::vector<void*> remappedVertices;
        size_t remappedVertexCount;

        switch (indices.stride)
        {
        case 4:
            remappedVertexCount = meshopt_generateVertexRemapMulti(&remap[0], (const unsigned int*)indices.data, indexCount, vertexCount, vertexStreams.data(), vertexStreams.size());
            meshopt_remapIndexBuffer((unsigned int*)remappedIndices, (const unsigned int*)indices.data, indexCount, &remap[0]);
            break;
        case 2:
            remappedVertexCount = meshopt_generateVertexRemapMulti(&remap[0], (const unsigned short*)indices.data, indexCount, vertexCount, vertexStreams.data(), vertexStreams.size());
            meshopt_remapIndexBuffer((unsigned short*)remappedIndices, (const unsigned short*)indices.data, indexCount, &remap[0]);
            break;
        case 1:
            remappedVertexCount = meshopt_generateVertexRemapMulti(&remap[0], (const unsigned char*)indices.data, indexCount, vertexCount, vertexStreams.data(), vertexStreams.size());
            meshopt_remapIndexBuffer((unsigned char*)remappedIndices, (const unsigned char*)indices.data, indexCount, &remap[0]);
            break;
        default:
            assert(false);
            break;
        }

        void* posVertices = nullptr;
        size_t posStride = 0;
        for (size_t i = 0; i < vertexStreams.size(); i++)
        {
            void* vertices = VTNA_ALLOC(vertexStreams[i].stride * remappedVertexCount);
            meshopt_remapVertexBuffer(vertices, vertexStreams[i].data, vertexCount, vertexStreams[i].stride, &remap[0]);
            remappedVertices.push_back(vertices);
            if (vertexTypes[i] == cgltf_attribute_type_position)
            {
                posVertices = vertices;
                posStride = vertexStreams[i].stride;
            }
        }

        size_t maxVertices = 64;
        size_t maxTriangles = 124;
        // const float coneWeight = 0.5f;

        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto resourceCache = ResourceCache::GetInstance();

        mesh->mpRenderer = pRenderer;

        if (indices.stride == 1)
        {
            uint16_t* data = (uint16_t*)VTNA_ALLOC(sizeof(uint16_t) * indexCount);
            for (uint32_t i = 0; i < indexCount; i++)
            {
                data[i] = ((const char*)indices.data)[i];
            }
            indices.stride = 2;
            VTNA_FREE((void*)indices.data);
            indices.data = data;
        }
        
        mesh->mIndexBuffer = resourceCache->GetSceneBuffer("Model(" + mFile + " " + name + ")_IndexBuffer", remappedIndices, (uint32_t)indices.stride * (uint32_t)indexCount);
        mesh->mIndexBufferFormat = indices.stride == 4 ? RHI::ERHIFormat::R32UI : RHI::ERHIFormat::R16UI;
        mesh->mIndexCount = (uint32_t)indexCount;
        mesh->mVertexCount = (uint32_t)remappedVertexCount;

        for (size_t i = 0; i < vertexTypes.size(); i++)
        {
            switch (vertexTypes[i])
            {
            case cgltf_attribute_type_position:
                mesh->mPositionBuffer = resourceCache->GetSceneBuffer("Model(" + mFile + " " + name + ")_PositionBuffer", remappedVertices[i], (uint32_t)vertexStreams[i].stride * (uint32_t)remappedVertexCount);
                break;
            case cgltf_attribute_type_texcoord:
                mesh->mTexCoordBuffer = resourceCache->GetSceneBuffer("Model(" + mFile + " " + name + ")_TexCoordBuffer", remappedVertices[i], (uint32_t)vertexStreams[i].stride * (uint32_t)remappedVertexCount);
                break;
            case cgltf_attribute_type_normal:
                mesh->mNormalBuffer = resourceCache->GetSceneBuffer("Model(" + mFile + " " + name + ")_NormalBuffer", remappedVertices[i], (uint32_t)vertexStreams[i].stride * (uint32_t)remappedVertexCount);
                break;
            case cgltf_attribute_type_tangent:
                mesh->mTangentBuffer = resourceCache->GetSceneBuffer("Model(" + mFile + " " + name + ")_TangentBuffer", remappedVertices[i], (uint32_t)vertexStreams[i].stride * (uint32_t)remappedVertexCount);
                break;
            default:
                break;
            }
        }

        mesh->Create();

        mpWorld->AddObject(mesh);

        VTNA_FREE((void*)indices.data);
        for (size_t i = 0; i < vertexStreams.size(); i++)
        {
            VTNA_FREE((void*)vertexStreams[i].data);
        }
        VTNA_FREE(remappedIndices);
        for (size_t i = 0; i < remappedVertices.size(); i++)
        {
            VTNA_FREE(remappedVertices[i]);
        }

        return mesh;
    }

    Scene::Animation *ModelLoader::LoadAnimation(const cgltf_data *data, const cgltf_animation *gltfAnimation)
    {
        Scene::Animation* animation = new Scene::Animation(gltfAnimation->name ? gltfAnimation->name : "");
        animation->mChannels.reserve(gltfAnimation->channels_count);

        for (cgltf_size i = 0; i < gltfAnimation->channels_count; i++)
        {
            const cgltf_animation_channel* gltfChannel = &gltfAnimation->channels[i];

            Scene::FAnimationChannel channel;
            channel.TargetNode = GetNodeIndex(data, gltfChannel->target_node);

            switch (gltfChannel->target_path)
            {
            case cgltf_animation_path_type_translation:
                channel.Mode = Scene::EAnimationChannelMode::Translation;
                break;
            case cgltf_animation_path_type_rotation:
                channel.Mode = Scene::EAnimationChannelMode::Rotatioin;
                break;
            case cgltf_animation_path_type_scale:
                channel.Mode = Scene::EAnimationChannelMode::Scale;
                break;
            default:
                assert(false);
                break; 
            };

            const cgltf_animation_sampler* animSampler = gltfChannel->sampler;
            assert(animSampler->interpolation == cgltf_interpolation_type_linear);

            cgltf_accessor* timeAccessor = animSampler->input;
            cgltf_accessor* valueAccessor = animSampler->output;    
            assert(timeAccessor->count == valueAccessor->count);

            cgltf_size keyFrameCount = timeAccessor->count;
            channel.KeyFrames.reserve(keyFrameCount);

            char* timeData = (char*)timeAccessor->buffer_view->buffer->data + timeAccessor->buffer_view->offset + timeAccessor->offset;
            char* valueData = (char*)valueAccessor->buffer_view->buffer->data + valueAccessor->buffer_view->offset + valueAccessor->offset;

            for (cgltf_size k = 0; k < keyFrameCount; k++)
            {
                float time;
                float4 value;

                memcpy(&time, timeData + timeAccessor->stride * k, timeAccessor->stride);
                memcpy(&value, valueData + valueAccessor->stride * k, valueAccessor->stride);

                channel.KeyFrames.push_back(std::make_pair(time, value));
            }
            animation->mChannels.push_back(channel);

            assert(timeAccessor->has_min && timeAccessor->has_max);
            float duration = timeAccessor->max[0] - timeAccessor->min[0];
            animation->mTimeDuration = std::max(animation->mTimeDuration, duration);
        }
        return animation;
    }

    Scene::Skeleton *ModelLoader::LoadSkeleton(const cgltf_data *data, const cgltf_skin *gltfSkin)
    {
        if (gltfSkin == nullptr)
        {
            return nullptr;
        }

        Scene::Skeleton* skeleton = new Scene::Skeleton(gltfSkin->name ? gltfSkin->name : "");
        skeleton->mJoints.resize(gltfSkin->joints_count);
        skeleton->mInverseBindMatrices.resize(gltfSkin->joints_count);
        skeleton->mJointMatrices.resize(gltfSkin->joints_count);

        for (cgltf_size i = 0; i < gltfSkin->joints_count; i++)
        {
            skeleton->mJoints[i] = GetNodeIndex(data, gltfSkin->joints[i]);
        }

        const cgltf_accessor* accessor = gltfSkin->inverse_bind_matrices;
        assert(accessor->count == gltfSkin->joints_count);

        uint32_t size = (uint32_t) accessor->stride * (uint32_t) accessor->count;

        std::vector<float4x4> inverseBindMatrices(accessor->count);
        memcpy(inverseBindMatrices.data(), (char*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset, size);

        for (cgltf_size i = 0; i < accessor->count; i++)
        {
            float3 translation;
            float4 rotatioin;
            float3 scale;
            Decompose(inverseBindMatrices[i], translation, rotatioin, scale);

            translation.z = -translation.z;
            rotatioin.z = -rotatioin.z;
            rotatioin.w = -rotatioin.w;

            float4x4 T = translation_matrix(translation);
            float4x4 R = rotation_matrix(rotatioin);
            float4x4 S = scaling_matrix(scale);

            skeleton->mInverseBindMatrices[i] = mul(T, mul(R, S));
        }
        return skeleton;
    }

    Scene::FSkeletonMeshData *ModelLoader::LoadSkeletalMeshData(const cgltf_primitive *primitive, const std::string &name)
    {
        auto mesh = new Scene::FSkeletonMeshData;
        mesh->Name = mFile + "_" + name;
        mesh->Material.reset(LoadMaterial(primitive->material));

        Assets::ResourceCache* cache = Assets::ResourceCache::GetInstance();

        size_t indexCount;
        meshopt_Stream indices = LoadBufferStream(primitive->indices, false, indexCount);

        mesh->IndexBuffer = cache->GetSceneBuffer("Model(" + mFile + "_" + name + ")_IndexBuffer", indices.data, (uint32_t)indices.stride * (uint32_t)indexCount);
        mesh->IndexBufferFormat = indices.stride == 4 ? RHI::ERHIFormat::R32UI : RHI::ERHIFormat::R16UI;
        mesh->IndexCount = (uint32_t)indexCount;

        size_t vertexCount;
        meshopt_Stream vertices;

        for (cgltf_size i = 0; i < primitive->attributes_count; i++)
        {
            switch (primitive->attributes[i].type)
            {
            case cgltf_attribute_type_position:
            {
                vertices = LoadBufferStream(primitive->attributes[i].data, true, vertexCount);
                mesh->StaticPositionBuffer = cache->GetSceneBuffer("Model(" + mFile + "_" + name + ")_PositionBuffer", vertices.data, (uint32_t)vertices.stride * (uint32_t)vertexCount);
                {
                    float3 min = float3(primitive->attributes[i].data->min);
                    min.z = -min.z;
                    float3 max = float3(primitive->attributes[i].data->max);
                    max.z = -max.z;

                    float3 center = (min + max) * 0.5f;
                    float radius = length(max - min) * 0.5f;

                    mesh->Center = center;
                    mesh->Radius = radius;
                }
                break;
            }
            case cgltf_attribute_type_texcoord:
            {
                if (primitive->attributes[i].index == 0)
                {
                    vertices = LoadBufferStream(primitive->attributes[i].data, false, vertexCount);
                    mesh->TexCoordBuffer = cache->GetSceneBuffer("Model(" + mFile + "_" + name + ")_TexCoordBuffer", vertices.data, (uint32_t)vertices.stride * (uint32_t)vertexCount);
                }
                break;
            }
            case cgltf_attribute_type_normal:
            {
                vertices = LoadBufferStream(primitive->attributes[i].data, true, vertexCount);
                mesh->StaticNormalBuffer = cache->GetSceneBuffer("Model(" + mFile + "_" + name + ")_NormalBuffer", vertices.data, (uint32_t)vertices.stride * (uint32_t)vertexCount);
                break;
            }
            case cgltf_attribute_type_tangent:
            {
                vertices = LoadBufferStream(primitive->attributes[i].data, false, vertexCount);
                mesh->StaticTangentBuffer = cache->GetSceneBuffer("Model(" + mFile + "_" + name + ")_TangentBuffer", vertices.data, (uint32_t)vertices.stride * (uint32_t)vertexCount);
                break;
            }
            case cgltf_attribute_type_joints:
            {
                const cgltf_accessor* accessor = primitive->attributes[i].data;

                std::vector<ushort4> jointIDs;
                jointIDs.reserve(accessor->count * 4);

                for (cgltf_size j = 0; j < accessor->count; j++)
                {
                    cgltf_uint id[4];
                    cgltf_accessor_read_uint(accessor, j, id, 4);
                    jointIDs.push_back(ushort4(id[0], id[1], id[2], id[3]));
                }
                mesh->JointIDBuffer = cache->GetSceneBuffer("Model(" + mFile + "_" + name + ")_JointIDBuffer", jointIDs.data(), sizeof(ushort4) * (uint32_t)accessor->count);
                break;
            }
            case cgltf_attribute_type_weights:
            {
                const cgltf_accessor* accessor = primitive->attributes[i].data;

                std::vector<float4> jointWeights;
                jointWeights.reserve(accessor->count);

                for (cgltf_size j = 0; j < accessor->count; j++)
                {
                    cgltf_float weight[4];
                    cgltf_accessor_read_float(accessor, j, weight, 4);
                    jointWeights.push_back(float4(weight));
                }
                mesh->JointWeightBuffer = cache->GetSceneBuffer("Model(" + mFile + "_" + name + ")_JointWeightBuffer", jointWeights.data(), sizeof(float4) * (uint32_t)accessor->count);
                break;
            }
            default:
                break;
            };
        }
        mesh->VertexCount = (uint32_t)vertexCount;
        return mesh;
    }

    Scene::FSkeletonMeshNode *ModelLoader::LoadSkeletalMeshNode(const cgltf_data *data, cgltf_node *gltfNode)
    {
        auto node = new Scene::FSkeletonMeshNode;
        node->ID = GetNodeIndex(data, gltfNode);
        node->Name = gltfNode->name ? gltfNode->name : fmt::format("Node_{}", node->ID).c_str();
        node->Parent = gltfNode->parent ? GetNodeIndex(data, gltfNode->parent) : -1;

        for (cgltf_size i = 0; i < gltfNode->children_count; i++)
        {
            node->Children.push_back(GetNodeIndex(data, gltfNode->children[i]));
        }

        if (gltfNode->has_matrix)
        {
            float4x4 matrix = float4x4(gltfNode->matrix);
            Decompose(matrix, node->Translation, node->Rotation, node->Scale);
        }
        else
        {
            node->Translation = float3(gltfNode->translation);
            node->Rotation = float4(gltfNode->rotation);
            node->Scale = float3(gltfNode->scale);
        }

        node->Translation.z *= -1;
        node->Rotation.z *= -1;
        node->Rotation.w *= -1;

        if (gltfNode->mesh)
        {
            bool vertexSkinning = gltfNode->skin != nullptr;
            uint32_t meshIndex = GetMeshIndex(data, gltfNode->mesh);
            bool bFrontFaceCCW = IsFrontFaceCCW(gltfNode);

            for (cgltf_size i = 0; i < gltfNode->mesh->primitives_count; i++)
            {
                std::string name = fmt::format("Mesh_{}_{} : {}", meshIndex, i, (gltfNode->mesh->name ? gltfNode->mesh->name : "")).c_str();

                auto mesh = LoadSkeletalMeshData(&gltfNode->mesh->primitives[i], name);
                mesh->NodeID = node->ID;
                mesh->Material->mbSkeletalAnim = vertexSkinning;
                mesh->Material->mbFrontFaceCCW = bFrontFaceCCW;

                node->Meshes.emplace_back(mesh);
            }
        }
        return node;
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