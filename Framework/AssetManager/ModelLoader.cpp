#include "AssetManager/ModelLoader.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "AssetManager/AssetManager.hpp"
#include "AssetManager/AssetFormatMapping.hpp"

#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetCooker/AssetValidate.hpp"
#include "AssetCooker/VAssetTypes.hpp"

#include "Scene/SceneComponent/Animation.hpp"
#include "Scene/SceneComponent/Animation.hpp"
#include "Scene/SceneComponent/StaticMesh.hpp"
#include "Scene/SceneComponent/SkeletalMesh.hpp"
#include "ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"

#include "Utilities/Log.hpp"
#include "Utilities/Memory.hpp"
#include "Utilities/String.hpp"
#include "Utilities/Math.hpp"

#include "Common/Meshlet.hlsli"   // FMeshlet:CPU/GPU 共享的 meshlet 剔除结构(36B)

#include <cgltf/cgltf.h>

#include <tinyxml2/tinyxml2.h>
#include <meshoptimizer.h>

#include <cassert>

inline float3 StrToFloat3(const eastl::string& str)
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

float4x4 LocalTransform(const Assets::FAssetNode& node)
{
    const float4x4 T = translation_matrix(node.Translation);
    const float4x4 R = rotation_matrix(node.Rotation);
    const float4x4 S = scaling_matrix(node.Scale);
    return mul(T, mul(R, S));
}

namespace Assets
{
    FModelLoader::FModelLoader(Scene::FWorld *pWorld)
    {
        m_pWorld = pWorld;

        float4x4 T = translation_matrix(m_Position);
        float4x4 R = rotation_matrix(m_Rotation);
        float4x4 S = scaling_matrix(m_Scale);

        m_MtxWorld = mul(T, mul(R, S));
    }

    FModelLoader::~FModelLoader()
    {
    }

    void FModelLoader::LoadModelSettings(tinyxml2::XMLElement *element)
    {
        m_File = element->FindAttribute("File")->Value();

        const tinyxml2::XMLAttribute* positionAttr = element->FindAttribute("Position");
        if (positionAttr)
        {
            m_Position = StrToFloat3(positionAttr->Value());
        }
        const tinyxml2::XMLAttribute* rotationAttr = element->FindAttribute("Rotation");
        if (rotationAttr)
        {
            m_Rotation = RotationQuat(StrToFloat3(rotationAttr->Value()));
        }
        const tinyxml2::XMLAttribute* scaleAttr = element->FindAttribute("Scale");
        if (scaleAttr)
        {
            m_Scale = StrToFloat3(scaleAttr->Value());
        }

        float4x4 T = translation_matrix(m_Position);
        float4x4 R = rotation_matrix(m_Rotation);
        float4x4 S = scaling_matrix(m_Scale);
        m_MtxWorld = mul(T, mul(R, S));
    }

    void FModelLoader::LoadGLTF(const char *gltfFile)
    {
        eastl::string file = Core::FVultanaEngine::GetEngineInstance()->GetAssetsPath() + (gltfFile ? gltfFile : m_File);

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
            Scene::FSkeletalMesh* mesh = new Scene::FSkeletalMesh(m_File);
            mesh->m_pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
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

    void FModelLoader::RequestLoad()
    {
        const eastl::string srcPath = Core::FVultanaEngine::GetEngineInstance()->GetAssetsPath() + m_File;

        auto self = shared_from_this();
        Core::FVultanaEngine::GetEngineInstance()->GetAssetManager()->RequestModel(srcPath,
        [self](Assets::FModelLoadResult&& result)
        {
            self->OnModelReady(eastl::move(result));
        });
    }

    void FModelLoader::LoadStaticModel(const FModelCookedData& model)
    {
        // Compose the XML instance transform (m_MtxWorld) with the node hierarchy — the legacy
        // LoadStaticMeshNode DFS equivalent (parentMtx * localToParent).
        eastl::vector<float4x4> world(model.Nodes.size(), float4x4({identity}));
        eastl::vector<int32_t> stack;

        eastl::vector<uint8_t> visited(model.Nodes.size(), 0);
        for (int32_t root : model.RootNodes)
        {
            world[static_cast<size_t>(root)] = mul(m_MtxWorld, LocalTransform(model.Nodes[static_cast<size_t>(root)]));
            visited[static_cast<size_t>(root)] = 1;
            stack.push_back(root);
        }

        while (!stack.empty())
        {
            const int32_t n = stack.back();
            stack.pop_back();
            const FAssetNode& node = model.Nodes[static_cast<size_t>(n)];
            for (int32_t child : node.Children)
            {
                world[static_cast<size_t>(child)] = mul(world[static_cast<size_t>(n)], LocalTransform(model.Nodes[static_cast<size_t>(child)]));
                visited[static_cast<size_t>(child)] = 1;
                stack.push_back(child);
            }
        }

        for (size_t n = 0; n < model.Nodes.size(); ++n)
        {
            if (!visited[n])
                continue;
            const FAssetNode& node = model.Nodes[n];
            if (node.Primitives.empty())
                continue;

            float3 pos;
            float4 rot;
            float3 scale;
            Decompose(world[n], pos, rot, scale);

            for (int32_t prim : node.Primitives)
            {
                const FMeshCookedData& cooked = model.Meshes[static_cast<size_t>(prim)];
                const FAssetMaterial* assetMat =
                    (cooked.MaterialIndex >= 0) ? &model.Materials[static_cast<size_t>(cooked.MaterialIndex)] : nullptr;
                BuildStaticMesh(cooked, assetMat, node.bFrontFaceCCW, pos, rot, scale);
            }
        }
    }

    void FModelLoader::LoadSkeletalModel(const FModelCookedData& model)
    {
        auto* pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
        Scene::FSkeletalMesh* skeletalMesh = new Scene::FSkeletalMesh(m_File);
        skeletalMesh->m_pRenderer = pRenderer;

        // Animation drives node transforms (FSkeletalMesh::Tick calls m_pAnimation->Update).
        // Animated-but-not-skinned models (BusterDrone) have an animation but no skeleton;
        // mesh->Material->IsVertexSkinned() stays false, so no compute skinning runs.
        if (!model.Animations.empty())
        {
            skeletalMesh->m_pAnimation.reset(BuildAnimation(model.Animations[0]));
        }
        if (!model.Skeleton.Joints.empty())
        {
            skeletalMesh->m_pSkeleton.reset(BuildSkeleton(model.Skeleton));
        }

        skeletalMesh->m_Nodes.reserve(model.Nodes.size());
        for (size_t i = 0; i < model.Nodes.size(); ++i)
        {
            skeletalMesh->m_Nodes.emplace_back(BuildSkeletalNode(model.Nodes[i], static_cast<int32_t>(i), model));
        }
        for (int32_t root : model.RootNodes)
        {
            skeletalMesh->m_RootNodes.push_back(static_cast<uint32_t>(root));
        }

        skeletalMesh->SetPosition(m_Position);
        skeletalMesh->SetRotation(m_Rotation);
        skeletalMesh->SetScale(m_Scale);
        skeletalMesh->Create();
        m_pWorld->AddObject(skeletalMesh);
    }

    void FModelLoader::OnModelReady(Assets::FModelLoadResult&& result)
    {
        if (!result.bSuccess || !result.Payload)
        {
            VTNA_LOG_ERROR("[ModelLoader::Load] failed to load {}", result.SrcPath);
            return;
        }
        m_PendingModel = eastl::move(result.Payload);
        Core::FVultanaEngine::GetEngineInstance()->GetAssetManager()->RequestModelTextures(*m_PendingModel, 
        [self = shared_from_this()]()
        { 
            self->OnModelTexturesReady(); 
        });
    }

    void FModelLoader::OnModelTexturesReady()
    {
        const FModelCookedData& model = *m_PendingModel;
        if (!model.Animations.empty() || !model.Skeleton.Joints.empty())
        {
            LoadSkeletalModel(model);
        }
        else
        {
            LoadStaticModel(model);
        }
        m_PendingModel.reset();
    }

    Scene::FStaticMesh* FModelLoader::BuildStaticMesh(const FMeshCookedData& cooked, const FAssetMaterial* assetMat, bool bFrontFaceCCW, const float3& pos, const quaternion& rot, const float3& scale)
    {
        Scene::FStaticMesh* mesh = new Scene::FStaticMesh(m_File + " " + cooked.Name);
        mesh->m_pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
        auto* cache = FResourceCache::GetInstance();
        const eastl::string key = "Model(" + m_File + " " + cooked.Name + ")"; // same key space as the legacy path

        mesh->m_IndexBuffer = cache->GetSceneBuffer(key + "_IndexBuffer", cooked.IndexData.data(), static_cast<uint32_t>(cooked.IndexData.size()));
        mesh->m_IndexBufferFormat = ToIndexFormat(cooked.IndexStride);
        mesh->m_IndexCount = cooked.IndexCount;
        mesh->m_VertexCount = cooked.VertexCount;

        for (const FMeshCookedData::FStream& stream : cooked.Streams)
        {
            const uint32_t bytes = static_cast<uint32_t>(stream.Data.size());
            switch (stream.Type)
            {
            case EVertexStreamType::Position:
                mesh->m_PositionBuffer = cache->GetSceneBuffer(key + "_PositionBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::TexCoord:
                mesh->m_TexCoordBuffer = cache->GetSceneBuffer(key + "_TexCoordBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::Normal:
                mesh->m_NormalBuffer = cache->GetSceneBuffer(key + "_NormalBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::Tangent:
                mesh->m_TangentBuffer = cache->GetSceneBuffer(key + "_TangentBuffer", stream.Data.data(), bytes);
                break;
            default:
                break; // Joints/Weights never reach the static path
            }
        }

        mesh->m_MeshletCount = cooked.MeshletCount;
        mesh->m_MeshletBuffer = cache->GetSceneBuffer(key + "_MeshletBuffer", cooked.MeshletBounds.data(), static_cast<uint32_t>(sizeof(FMeshlet) * cooked.MeshletBounds.size()));
        mesh->m_MeshletIndicesBuffer = cache->GetSceneBuffer(key + "_MeshletIndicesBuffer", cooked.MeshletIndexData.data(), static_cast<uint32_t>(cooked.MeshletIndexData.size()));
        mesh->m_MeshletVertexBuffer = cache->GetSceneBuffer(key + "_MeshletVertexBuffer", cooked.MeshletVertexData.data(), static_cast<uint32_t>(cooked.MeshletVertexData.size()));

        mesh->m_Center = cooked.Center;
        mesh->m_Radius = cooked.Radius;

        mesh->m_pMaterial.reset(BuildMaterial(assetMat, /*bSkeletalAnim=*/false, bFrontFaceCCW));
        mesh->SetPosition(pos);
        mesh->SetRotation(rot);
        mesh->SetScale(scale);
        mesh->Create();
        m_pWorld->AddObject(mesh);
        return mesh;
    }

    Scene::FSkeletalMeshNode* FModelLoader::BuildSkeletalNode(const FAssetNode& node, int32_t nodeIndex, const FModelCookedData& model)
    {
        Scene::FSkeletalMeshNode* skeletalNode = new Scene::FSkeletalMeshNode;
        skeletalNode->Name = node.Name;
        skeletalNode->ID = static_cast<uint32_t>(nodeIndex);
        skeletalNode->Parent = static_cast<uint32_t>(node.Parent);
        for (int32_t child : node.Children)
            skeletalNode->Children.push_back(static_cast<uint32_t>(child));
        skeletalNode->Translation = node.Translation;
        skeletalNode->Rotation = node.Rotation;
        skeletalNode->Scale = node.Scale;

        for (int32_t prim : node.Primitives)
        {
            const FMeshCookedData& cooked = model.Meshes[static_cast<size_t>(prim)];
            const FAssetMaterial* assetMat =
                (cooked.MaterialIndex >= 0) ? &model.Materials[static_cast<size_t>(cooked.MaterialIndex)] : nullptr;
            // bSkeletalAnim is per-mesh (node->skin at cook time); bFrontFaceCCW is per-node.
            Scene::FSkeletalMeshData* meshData = BuildSkeletalMeshData(cooked, assetMat, cooked.bSkeletalAnim, node.bFrontFaceCCW);
            meshData->NodeID = skeletalNode->ID;
            skeletalNode->Meshes.emplace_back(meshData);
        }
        return skeletalNode;
    }

    Scene::FSkeletalMeshData* FModelLoader::BuildSkeletalMeshData(const FMeshCookedData& cooked, const FAssetMaterial* assetMat, bool bSkeletalAnim, bool bFrontFaceCCW)
    {
        Scene::FSkeletalMeshData* mesh = new Scene::FSkeletalMeshData;
        mesh->Name = m_File + "_" + cooked.Name;
        auto* cache = FResourceCache::GetInstance();
        const eastl::string key = "Model(" + m_File + "_" + cooked.Name + ")"; // underscore: legacy skeletal key space

        mesh->IndexBuffer = cache->GetSceneBuffer(key + "_IndexBuffer", cooked.IndexData.data(), static_cast<uint32_t>(cooked.IndexData.size()));
        mesh->IndexBufferFormat = ToIndexFormat(cooked.IndexStride);
        mesh->IndexCount = cooked.IndexCount;
        mesh->VertexCount = cooked.VertexCount;
        mesh->Center = cooked.Center;
        mesh->Radius = cooked.Radius;

        for (const FMeshCookedData::FStream& stream : cooked.Streams)
        {
            const uint32_t bytes = static_cast<uint32_t>(stream.Data.size());
            switch (stream.Type)
            {
            case EVertexStreamType::Position:
                mesh->StaticPositionBuffer = cache->GetSceneBuffer(key + "_PositionBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::TexCoord:
                mesh->TexCoordBuffer = cache->GetSceneBuffer(key + "_TexCoordBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::Normal:
                mesh->StaticNormalBuffer = cache->GetSceneBuffer(key + "_NormalBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::Tangent:
                mesh->StaticTangentBuffer = cache->GetSceneBuffer(key + "_TangentBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::Joints:
                mesh->JointIDBuffer = cache->GetSceneBuffer(key + "_JointIDBuffer", stream.Data.data(), bytes);
                break;
            case EVertexStreamType::Weights:
                mesh->JointWeightBuffer = cache->GetSceneBuffer(key + "_JointWeightBuffer", stream.Data.data(), bytes);
                break;
            }
        }
        mesh->Material.reset(BuildMaterial(assetMat, bSkeletalAnim, bFrontFaceCCW));
        return mesh;
    }

    FMeshMaterial* FModelLoader::BuildMaterial(const FAssetMaterial* assetMat, bool bSkeletalAnim, bool bFrontFaceCCW)
    {
        FMeshMaterial* material = new FMeshMaterial;
        material->m_bSkeletalAnim = bSkeletalAnim;
        material->m_bFrontFaceCCW = bFrontFaceCCW;
        if (assetMat == nullptr)
        {
            return material; // untextured primitive: engine defaults
        }
        material->m_Name = assetMat->Name;
        material->m_WorkFlow = ToEngineWorkFlow(assetMat->WorkFlow);
        material->m_AlbedoColor = assetMat->AlbedoColor;
        material->m_Metallic = assetMat->Metallic;
        material->m_Roughness = assetMat->Roughness;
        material->m_DiffuseColor = assetMat->DiffuseColor;
        material->m_SpecularColor = assetMat->SpecularColor;
        material->m_Glossiness = assetMat->Glossiness;
        material->m_EmissiveColor = assetMat->EmissiveColor;
        material->m_AlphaCutout = assetMat->AlphaCutout;
        material->m_bAlphaTest = assetMat->bAlphaTest;
        material->m_bAlphaBlend = assetMat->bAlphaBlend;
        material->m_bDoubleSided = assetMat->bDoubleSided;

        auto* textureCache = FResourceCache::GetInstance();
        auto resolveTexture = [&](const FAssetTextureRef& ref) -> RenderResources::FTexture2D*
        {
            const eastl::string srcRel = FAssetPathUtils::GetTextureSourcePath(ref.RelativePath);
            if (FAssetPathUtils::IsDDSSource(srcRel))
            {
                return textureCache->GetTexture2D(Core::FVultanaEngine::GetEngineInstance()->GetAssetsPath() + srcRel, ref.bSRGB);
            }
            return textureCache->GetTexture2DProductReady(FAssetPathUtils::GetContentRootPath() + ref.RelativePath);
        };

        for (const FAssetTextureRef& ref : assetMat->Textures)
        {
            switch (ref.Usage)
            {
            case ETextureUsage::BaseColor:
                material->m_pAlbedoTexture = resolveTexture(ref);
                material->m_MaterialCB.AlbedoTexture = BuildTextureInfo(material->m_pAlbedoTexture, ref);
                break;
            case ETextureUsage::Normal:
                material->m_pNormalTexture = resolveTexture(ref);
                material->m_MaterialCB.NormalTexture = BuildTextureInfo(material->m_pNormalTexture, ref);
                break;
            case ETextureUsage::Emissive:
                material->m_pEmissiveTexture = resolveTexture(ref);
                material->m_MaterialCB.EmissiveTexture = BuildTextureInfo(material->m_pEmissiveTexture, ref);
                break;
            case ETextureUsage::AmbientOcclusion:
                material->m_pAOTexture = resolveTexture(ref);
                material->m_MaterialCB.AmbientOcclusionTexture = BuildTextureInfo(material->m_pAOTexture, ref);
                break;
            case ETextureUsage::MetallicRoughness:
                material->m_pMetallicRoughTexture = resolveTexture(ref);
                material->m_MaterialCB.MetallicRoughnessTexture = BuildTextureInfo(material->m_pMetallicRoughTexture, ref);
                break;
            case ETextureUsage::Diffuse:
                material->m_pDiffuseTexture = resolveTexture(ref);
                material->m_MaterialCB.DiffuseTexture = BuildTextureInfo(material->m_pDiffuseTexture, ref);
                break;
            case ETextureUsage::SpecularGlossiness:
                material->m_pSpecularGlossinessTexture = resolveTexture(ref);
                material->m_MaterialCB.SpecularGlossinessTexture = BuildTextureInfo(material->m_pSpecularGlossinessTexture, ref);
                break;
            default:
                break;
            }
        }
        return material;
    }

    FMaterialTextureInfo FModelLoader::BuildTextureInfo(const RenderResources::FTexture2D* texture, const FAssetTextureRef& ref)
    {
        FMaterialTextureInfo info;
        if (texture)
        {
            info.Index = texture->GetSRV()->GetHeapIndex();
            info.Width = texture->GetTexture()->GetDesc().Width;
            info.Height = texture->GetTexture()->GetDesc().Height;
            if (ref.bHasTransform)
            {
                info.IsTransform = true;
                info.Offset = ref.Offset;
                info.Scale = ref.Scale;
                info.Rotation = ref.Rotation;
            }
        }
        return info;
    }

    Scene::FAnimation* FModelLoader::BuildAnimation(const FAssetAnimation& anim)
    {
        Scene::FAnimation* animation = new Scene::FAnimation(anim.Name);
        animation->m_Channels.reserve(anim.Channels.size());
        for (const FAssetAnimationChannel& channel : anim.Channels)
        {
            Scene::FAnimationChannel out;
            out.TargetNode = static_cast<uint32_t>(channel.TargetNode);
            out.Mode = ToChannelMode(channel.Mode);
            out.KeyFrames = channel.KeyFrames;
            animation->m_Channels.push_back(eastl::move(out));
        }
        animation->m_TimeDuration = anim.TimeDuration;
        return animation;
    }

    Scene::FSkeleton* FModelLoader::BuildSkeleton(const FAssetSkeleton& skeleton)
    {
        Scene::FSkeleton* out = new Scene::FSkeleton("Skeleton");
        out->m_Joints.reserve(skeleton.Joints.size());
        for (int32_t joint : skeleton.Joints)
            out->m_Joints.push_back(static_cast<uint32_t>(joint));
        out->m_InverseBindMatrices = skeleton.InverseBindMatrices;
        out->m_JointMatrices.resize(skeleton.Joints.size());
        return out;
    }

    void FModelLoader::LoadStaticMeshNode(const cgltf_data* data, cgltf_node* node, const float4x4& parentMtx)
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
                Scene::FStaticMesh* mesh = LoadStaticMesh(&node->mesh->primitives[i], name, bFrontFaceCCW);
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

    Scene::FStaticMesh *FModelLoader::LoadStaticMesh(const cgltf_primitive *primitive, const eastl::string &name, bool bFrontFaceCCW)
    {
        Scene::FStaticMesh* mesh = new Scene::FStaticMesh(m_File + " " + name);
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

        eastl::vector<FMeshlet> meshletBounds(meshletCount);

        for (size_t i = 0; i < meshletCount; i++)
        {
            const meshopt_Meshlet& meshlet = meshlets[i];
            meshopt_Bounds meshoptBounds = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, (const float*)posVertices, remappedVertexCount, posStride);

            FMeshlet bound;
            bound.Center = float3(meshoptBounds.center);
            bound.Radius = meshoptBounds.radius;
            bound.AxisX = meshoptBounds.cone_axis_s8[0];
            bound.AxisY = meshoptBounds.cone_axis_s8[1];
            bound.AxisZ = meshoptBounds.cone_axis_s8[2];
            bound.Cutoff = meshoptBounds.cone_cutoff_s8;
            bound.VertexCount = meshlet.vertex_count;
            bound.TriangleCount = meshlet.triangle_count;
            bound.VertexOffset = meshlet.vertex_offset;
            bound.TriangleOffset = meshlet.triangle_offset;

            meshletBounds[i] = bound;
        }

        auto pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
        auto resourceCache = FResourceCache::GetInstance();

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
        mesh->m_MeshletBuffer = resourceCache->GetSceneBuffer("Model(" + m_File + " " + name + ")_MeshletBuffer", meshletBounds.data(), sizeof(FMeshlet) * (uint32_t)meshletBounds.size());
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

    Scene::FAnimation *FModelLoader::LoadAnimation(const cgltf_data *data, const cgltf_animation *gltfAnimation)
    {
        Scene::FAnimation* animation = new Scene::FAnimation(gltfAnimation->name ? gltfAnimation->name : "");
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

    Scene::FSkeleton *FModelLoader::LoadSkeleton(const cgltf_data *data, const cgltf_skin *gltfSkin)
    {
        if (gltfSkin == nullptr)
        {
            return nullptr;
        }

        Scene::FSkeleton* skeleton = new Scene::FSkeleton(gltfSkin->name ? gltfSkin->name : "");
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

    Scene::FSkeletalMeshNode *FModelLoader::LoadSkeletalMeshNode(const cgltf_data *data, cgltf_node *gltfNode)
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

    Scene::FSkeletalMeshData *FModelLoader::LoadSkeletalMeshData(const cgltf_primitive *primitive, const eastl::string &name)
    {
        Scene::FSkeletalMeshData* mesh = new Scene::FSkeletalMeshData;
        mesh->Name = m_File + "_" + name;
        mesh->Material.reset(LoadMaterial(primitive->material));

        Assets::FResourceCache* cache = Assets::FResourceCache::GetInstance();

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

    inline FMaterialTextureInfo LoadTextureInfo(const RenderResources::FTexture2D* texture, const cgltf_texture_view& textureView)
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

    FMeshMaterial *FModelLoader::LoadMaterial(const cgltf_material *gltfMaterial)
    {
        FMeshMaterial* material = new FMeshMaterial;
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
        material->m_bDoubleSided = gltfMaterial->double_sided;

        return material;
    }

    RenderResources::FTexture2D *FModelLoader::LoadTexture(const cgltf_texture_view& textureView, bool srgb)
    {
        if (textureView.texture == nullptr || textureView.texture->image->uri == nullptr) return nullptr;

        size_t lastSlash = m_File.find_last_of('/');
        eastl::string texturePath = Core::FVultanaEngine::GetEngineInstance()->GetAssetsPath() + m_File.substr(0, lastSlash + 1);
        Renderer::FRendererBase* pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
        auto texture = FResourceCache::GetInstance()->GetTexture2D(texturePath + textureView.texture->image->uri, srgb);
        return texture;
    }
}