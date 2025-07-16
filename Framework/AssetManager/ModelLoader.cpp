#include "ModelLoader.hpp"
#include "Scene/SceneComponent/Animation.hpp"
#include "Scene/SceneComponent/StaticMesh.hpp"
#include "Scene/SceneComponent/SkeletalMesh.hpp"
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

inline float3 strToFloat3(const eastl::string& str)
{
    eastl::vector<float> v;
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
        m_pWorld = pWorld;

        float4x4 T = translation_matrix(m_Position);
        float4x4 R = rotation_matrix(m_Rotation);
        float4x4 S = scaling_matrix(m_Scale);

        m_MtxWorld = mul(T, mul(R, S));
    }

    ModelLoader::~ModelLoader()
    {
    }

    void ModelLoader::LoadModelSettings(tinyxml2::XMLElement *element)
    {
        m_File = element->FindAttribute("File")->Value();

        const tinyxml2::XMLAttribute* positionAttr = element->FindAttribute("Position");
        if (positionAttr)
        {
            m_Position = strToFloat3(positionAttr->Value());
        }
        const tinyxml2::XMLAttribute* rotationAttr = element->FindAttribute("Rotation");
        if (rotationAttr)
        {
            m_Rotation = RotationQuat(strToFloat3(rotationAttr->Value()));
        }
        const tinyxml2::XMLAttribute* scaleAttr = element->FindAttribute("Scale");
        if (scaleAttr)
        {
            m_Scale = strToFloat3(scaleAttr->Value());
        }

        float4x4 T = translation_matrix(m_Position);
        float4x4 R = rotation_matrix(m_Rotation);
        float4x4 S = scaling_matrix(m_Scale);
        m_MtxWorld = mul(T, mul(R, S));
    }

    void ModelLoader::LoadGLTF(const char *gltfFile)
    {
        eastl::string file = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath() + (gltfFile ? gltfFile : m_File);

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
            Scene::SkeletalMesh* mesh = new Scene::SkeletalMesh(m_File);
            mesh->m_pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
            mesh->m_pAnimation.reset(LoadAnimation(data, &data->animations[0]));
            mesh->m_pSkeleton.reset(LoadSkeleton(data, &data->skins[0]));

            for (cgltf_size i = 0; i < data->nodes_count; i++)
            {
                mesh->m_Nodes.emplace_back(LoadSkeletalMeshNode(data, &data->nodes[i]));
            }
            for (cgltf_size i = 0; i < data->scene->nodes_count; i++)
            {
                mesh->m_RootNodes.push_back(GetNodeIndex(data, data->scene->nodes[i]));
            }

            mesh->SetPosition(m_Position);
            mesh->SetRotation(m_Rotation);
            mesh->SetScale(m_Scale);
            mesh->Create();
            m_pWorld->AddObject(mesh);
        }
        else
        {
            for (cgltf_size i = 0; i < data->scenes_count; i++)
            {
                for (cgltf_size node = 0; node < data->scenes[i].nodes_count; node++)
                {
                    LoadStaticMeshNode(data, data->scenes[i].nodes[node], m_MtxWorld);
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
                eastl::string name = fmt::format("Mesh_{}_{} : {}", meshIdx, i, (node->mesh->name ? node->mesh->name : "")).c_str();
                Scene::StaticMesh* mesh = LoadStaticMesh(&node->mesh->primitives[i], name, bFrontFaceCCW);
                mesh->m_pMaterial->m_bFrontFaceCCW = bFrontFaceCCW;
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

    Scene::StaticMesh *ModelLoader::LoadStaticMesh(const cgltf_primitive *primitive, const eastl::string &name, bool bFrontFaceCCW)
    {
        Scene::StaticMesh* mesh = new Scene::StaticMesh(m_File + " " + name);
        mesh->m_pMaterial.reset(LoadMaterial(primitive->material));

        size_t indexCount;
        meshopt_Stream indices = LoadBufferStream(primitive->indices, false, indexCount);

        size_t vertexCount;
        eastl::vector<meshopt_Stream> vertexStreams;
        eastl::vector<cgltf_attribute_type> vertexTypes;

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

                    mesh->m_Center = center;
                    mesh->m_Radius = radius;
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
        eastl::vector<unsigned int> remap(indexCount);

        void* remappedIndices = VTNA_ALLOC(indices.stride * indexCount);
        eastl::vector<void*> remappedVertices;
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
        const float coneWeight = 0.5f;
        size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, maxVertices, maxTriangles);

        eastl::vector<meshopt_Meshlet> meshlets(maxMeshlets);
        eastl::vector<unsigned int> meshletVertices(maxMeshlets * maxVertices);
        eastl::vector<unsigned char> meshletTriangles(maxMeshlets * maxTriangles * 3);

        size_t meshletCount;
        switch (indices.stride)
        {
        case 4:
            meshletCount = meshopt_buildMeshlets(meshlets.data(), meshletVertices.data(), meshletTriangles.data(), (const unsigned int*)remappedIndices, indexCount, (const float*)posVertices, remappedVertexCount, posStride, maxVertices, maxTriangles, coneWeight);
            break;
        case 2:
            meshletCount = meshopt_buildMeshlets(meshlets.data(), meshletVertices.data(), meshletTriangles.data(), (const unsigned short*)remappedIndices, indexCount, (const float*)posVertices, remappedVertexCount, posStride, maxVertices, maxTriangles, coneWeight);
            break;
        case 1:
            meshletCount = meshopt_buildMeshlets(meshlets.data(), meshletVertices.data(), meshletTriangles.data(), (const unsigned char*)remappedIndices, indexCount, (const float*)posVertices, remappedVertexCount, posStride, maxVertices, maxTriangles, coneWeight);
            break;
        default:
            assert(false);
            break;
        }

        const meshopt_Meshlet& lastMeshlet = meshlets[meshletCount - 1];
        meshletVertices.resize(lastMeshlet.vertex_offset + lastMeshlet.vertex_count);
        meshletTriangles.resize(lastMeshlet.triangle_offset + ((lastMeshlet.triangle_count * 3 + 3) & ~3));
        meshlets.resize(meshletCount);

        eastl::vector<unsigned short> meshletTriangles16;
        meshletTriangles16.reserve(meshletTriangles.size());
        for (size_t i = 0; i < meshletTriangles.size(); i++)
        {
            meshletTriangles16.push_back(meshletTriangles[i]);
        }

        struct MeshletBound
        {
            float3 Center;
            float Radius;

            union
            {
                struct 
                {
                    int8_t AxisX;
                    int8_t AxisY;
                    int8_t AxisZ;
                    int8_t Cutoff;
                };
                uint32_t Cone;
            };

            uint VertexCount;
            uint TriangleCount;

            uint vertexOffset;
            uint triangleOffset;
        };
        eastl::vector<MeshletBound> meshletBounds(meshletCount);

        for (size_t i = 0; i < meshletCount; i++)
        {
            const meshopt_Meshlet& meshlet = meshlets[i];
            meshopt_Bounds meshoptBounds = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, (const float*)posVertices, remappedVertexCount, posStride);

            MeshletBound bound;
            bound.Center = float3(meshoptBounds.center);
            bound.Radius = meshoptBounds.radius;    
            bound.AxisX = meshoptBounds.cone_axis_s8[0];
            bound.AxisY = meshoptBounds.cone_axis_s8[1];
            bound.AxisZ = meshoptBounds.cone_axis_s8[2];
            bound.Cutoff = meshoptBounds.cone_cutoff_s8;
            bound.VertexCount = meshlet.vertex_count;
            bound.TriangleCount = meshlet.triangle_count;
            bound.vertexOffset = meshlet.vertex_offset;
            bound.triangleOffset = meshlet.triangle_offset;

            meshletBounds[i] = bound;
        }

        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto resourceCache = ResourceCache::GetInstance();

        mesh->m_pRenderer = pRenderer;

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
        
        mesh->m_IndexBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_IndexBuffer", remappedIndices, (uint32_t)indices.stride * (uint32_t)indexCount);
        mesh->m_IndexBufferFormat = indices.stride == 4 ? RHI::ERHIFormat::R32UI : RHI::ERHIFormat::R16UI;
        mesh->m_IndexCount = (uint32_t)indexCount;
        mesh->m_VertexCount = (uint32_t)remappedVertexCount;

        for (size_t i = 0; i < vertexTypes.size(); i++)
        {
            switch (vertexTypes[i])
            {
            case cgltf_attribute_type_position:
                mesh->m_PositionBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_PositionBuffer", remappedVertices[i], (uint32_t)vertexStreams[i].stride * (uint32_t)remappedVertexCount);
                break;
            case cgltf_attribute_type_texcoord:
                mesh->m_TexCoordBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_TexCoordBuffer", remappedVertices[i], (uint32_t)vertexStreams[i].stride * (uint32_t)remappedVertexCount);
                break;
            case cgltf_attribute_type_normal:
                mesh->m_NormalBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_NormalBuffer", remappedVertices[i], (uint32_t)vertexStreams[i].stride * (uint32_t)remappedVertexCount);
                break;
            case cgltf_attribute_type_tangent:
                mesh->m_TangentBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_TangentBuffer", remappedVertices[i], (uint32_t)vertexStreams[i].stride * (uint32_t)remappedVertexCount);
                break;
            default:
                break;
            }
        }

        mesh->m_MeshletCount = (uint32_t)meshletCount;
        mesh->m_MeshletBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_MeshletBuffer", meshletBounds.data(), sizeof(MeshletBound) * (uint32_t)meshletBounds.size());
        mesh->m_MeshletIndicesBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_MeshletIndicesBuffer", meshletTriangles16.data(), sizeof(unsigned short) * (uint32_t)meshletTriangles16.size());
        mesh->m_MeshletVertexBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_MeshletVertexBuffer", meshletVertices.data(), sizeof(unsigned int) * (uint32_t)meshletVertices.size());

        mesh->Create();

        m_pWorld->AddObject(mesh);

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
        animation->m_Channels.reserve(gltfAnimation->channels_count);

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

                channel.KeyFrames.push_back(eastl::make_pair(time, value));
            }
            animation->m_Channels.push_back(channel);

            assert(timeAccessor->has_min && timeAccessor->has_max);
            float duration = timeAccessor->max[0] - timeAccessor->min[0];
            animation->m_TimeDuration = eastl::max(animation->m_TimeDuration, duration);
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
        skeleton->m_Joints.resize(gltfSkin->joints_count);
        skeleton->m_InverseBindMatrices.resize(gltfSkin->joints_count);
        skeleton->m_JointMatrices.resize(gltfSkin->joints_count);

        for (cgltf_size i = 0; i < gltfSkin->joints_count; i++)
        {
            skeleton->m_Joints[i] = GetNodeIndex(data, gltfSkin->joints[i]);
        }

        const cgltf_accessor* accessor = gltfSkin->inverse_bind_matrices;
        assert(accessor->count == gltfSkin->joints_count);

        uint32_t size = (uint32_t) accessor->stride * (uint32_t) accessor->count;

        eastl::vector<float4x4> inverseBindMatrices(accessor->count);
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

            skeleton->m_InverseBindMatrices[i] = mul(T, mul(R, S));
        }
        return skeleton;
    }

    Scene::FSkeletalMeshNode *ModelLoader::LoadSkeletalMeshNode(const cgltf_data *data, cgltf_node *gltfNode)
    {
        Scene::FSkeletalMeshNode* node = new Scene::FSkeletalMeshNode;
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
                eastl::string name = fmt::format("Mesh_{}_{} : {}", meshIndex, i, (gltfNode->mesh->name ? gltfNode->mesh->name : "")).c_str();

                Scene::FSkeletalMeshData* mesh = LoadSkeletalMeshData(&gltfNode->mesh->primitives[i], name);
                mesh->NodeID = node->ID;
                mesh->Material->m_bSkeletalAnim = vertexSkinning;
                mesh->Material->m_bFrontFaceCCW = bFrontFaceCCW;

                node->Meshes.emplace_back(mesh);
            }
        }
        return node;
    }

    Scene::FSkeletalMeshData *ModelLoader::LoadSkeletalMeshData(const cgltf_primitive *primitive, const eastl::string &name)
    {
        Scene::FSkeletalMeshData* mesh = new Scene::FSkeletalMeshData;
        mesh->Name = m_File + "_" + name;
        mesh->Material.reset(LoadMaterial(primitive->material));

        Assets::ResourceCache* cache = Assets::ResourceCache::GetInstance();

        size_t indexCount;
        meshopt_Stream indices = LoadBufferStream(primitive->indices, false, indexCount);

        mesh->IndexBuffer = cache->GetSceneBuffer("Model(" + m_File + "_" + name + ")_IndexBuffer", indices.data, (uint32_t)indices.stride * (uint32_t)indexCount);
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
                mesh->StaticPositionBuffer = cache->GetSceneBuffer("Model(" + m_File + "_" + name + ")_PositionBuffer", vertices.data, (uint32_t)vertices.stride * (uint32_t)vertexCount);
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
                    mesh->TexCoordBuffer = cache->GetSceneBuffer("Model(" + m_File + "_" + name + ")_TexCoordBuffer", vertices.data, (uint32_t)vertices.stride * (uint32_t)vertexCount);
                }
                break;
            }
            case cgltf_attribute_type_normal:
            {
                vertices = LoadBufferStream(primitive->attributes[i].data, true, vertexCount);
                mesh->StaticNormalBuffer = cache->GetSceneBuffer("Model(" + m_File + "_" + name + ")_NormalBuffer", vertices.data, (uint32_t)vertices.stride * (uint32_t)vertexCount);
                break;
            }
            case cgltf_attribute_type_tangent:
            {
                vertices = LoadBufferStream(primitive->attributes[i].data, false, vertexCount);
                mesh->StaticTangentBuffer = cache->GetSceneBuffer("Model(" + m_File + "_" + name + ")_TangentBuffer", vertices.data, (uint32_t)vertices.stride * (uint32_t)vertexCount);
                break;
            }
            case cgltf_attribute_type_joints:
            {
                const cgltf_accessor* accessor = primitive->attributes[i].data;

                eastl::vector<ushort4> jointIDs;
                jointIDs.reserve(accessor->count * 4);

                for (cgltf_size j = 0; j < accessor->count; j++)
                {
                    cgltf_uint id[4];
                    cgltf_accessor_read_uint(accessor, j, id, 4);
                    jointIDs.push_back(ushort4(id[0], id[1], id[2], id[3]));
                }
                mesh->JointIDBuffer = cache->GetSceneBuffer("Model(" + m_File + "_" + name + ")_JointIDBuffer", jointIDs.data(), sizeof(ushort4) * (uint32_t)accessor->count);
                break;
            }
            case cgltf_attribute_type_weights:
            {
                const cgltf_accessor* accessor = primitive->attributes[i].data;

                eastl::vector<float4> jointWeights;
                jointWeights.reserve(accessor->count);

                for (cgltf_size j = 0; j < accessor->count; j++)
                {
                    cgltf_float weight[4];
                    cgltf_accessor_read_float(accessor, j, weight, 4);
                    jointWeights.push_back(float4(weight));
                }
                mesh->JointWeightBuffer = cache->GetSceneBuffer("Model(" + m_File + "_" + name + ")_JointWeightBuffer", jointWeights.data(), sizeof(float4) * (uint32_t)accessor->count);
                break;
            }
            default:
                break;
            }
        }
        mesh->VertexCount = (uint32_t)vertexCount;
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
        material->m_Name = gltfMaterial->name ? gltfMaterial->name : "";

        if (gltfMaterial->has_pbr_metallic_roughness)
        {
            material->m_WorkFlow = MaterialWorkFlow::PBRMetallicRoughness;
            material->m_pAlbedoTexture = LoadTexture(gltfMaterial->pbr_metallic_roughness.base_color_texture, true);
            material->m_MaterialCB.AlbedoTexture = LoadTextureInfo(material->m_pAlbedoTexture, gltfMaterial->pbr_metallic_roughness.base_color_texture);
            material->m_pMetallicRoughTexture = LoadTexture(gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture, false);
            material->m_MaterialCB.MetallicRoughnessTexture = LoadTextureInfo(material->m_pMetallicRoughTexture, gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture);
            material->m_AlbedoColor = float3(gltfMaterial->pbr_metallic_roughness.base_color_factor);
            material->m_Metallic = gltfMaterial->pbr_metallic_roughness.metallic_factor;
            material->m_Roughness = gltfMaterial->pbr_metallic_roughness.roughness_factor;
        }
        else if (gltfMaterial->has_pbr_specular_glossiness)
        {
            material->m_WorkFlow = MaterialWorkFlow::PBRSpecularGlossiness;
            material->m_pDiffuseTexture = LoadTexture(gltfMaterial->pbr_specular_glossiness.diffuse_texture, true);
            material->m_MaterialCB.DiffuseTexture = LoadTextureInfo(material->m_pDiffuseTexture, gltfMaterial->pbr_specular_glossiness.diffuse_texture);
            material->m_pSpecularGlossinessTexture = LoadTexture(gltfMaterial->pbr_specular_glossiness.specular_glossiness_texture, false);
            material->m_MaterialCB.SpecularGlossinessTexture = LoadTextureInfo(material->m_pSpecularGlossinessTexture, gltfMaterial->pbr_specular_glossiness.specular_glossiness_texture);
            material->m_DiffuseColor = float3(gltfMaterial->pbr_specular_glossiness.diffuse_factor);
            material->m_SpecularColor = float3(gltfMaterial->pbr_specular_glossiness.specular_factor);
            material->m_Glossiness = gltfMaterial->pbr_specular_glossiness.glossiness_factor;
        }
        material->m_pNormalTexture = LoadTexture(gltfMaterial->normal_texture, false);
        material->m_MaterialCB.NormalTexture = LoadTextureInfo(material->m_pNormalTexture, gltfMaterial->normal_texture);
        material->m_pEmissiveTexture = LoadTexture(gltfMaterial->emissive_texture, true);
        material->m_MaterialCB.EmissiveTexture = LoadTextureInfo(material->m_pEmissiveTexture, gltfMaterial->emissive_texture);
        material->m_pAOTexture = LoadTexture(gltfMaterial->occlusion_texture, false);
        material->m_MaterialCB.AmbientOcclusionTexture = LoadTextureInfo(material->m_pAOTexture, gltfMaterial->occlusion_texture);

        material->m_EmissiveColor = float3(gltfMaterial->emissive_factor);
        material->m_AlphaCutout = gltfMaterial->alpha_cutoff;
        material->m_bAlphaTest = gltfMaterial->alpha_mode == cgltf_alpha_mode_mask;
        material->m_bAlphaBlend = gltfMaterial->alpha_mode == cgltf_alpha_mode_blend;
        material->m_bFrontFaceCCW = gltfMaterial->double_sided;

        return material;
    }

    RenderResources::Texture2D *ModelLoader::LoadTexture(const cgltf_texture_view& textureView, bool srgb)
    {
        if (textureView.texture == nullptr || textureView.texture->image->uri == nullptr) return nullptr;

        size_t lastSlash = m_File.find_last_of('/');
        eastl::string texturePath = Core::VultanaEngine::GetEngineInstance()->GetAssetsPath() + m_File.substr(0, lastSlash + 1);
        Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        auto texture = ResourceCache::GetInstance()->GetTexture2D(texturePath + textureView.texture->image->uri, srgb);
        return texture;
    }
}