#include "AssetCooker/AssimpImporter.hpp"
#include "AssetCooker/AssetPathUtils.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <assimp/scene.h>
#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/mesh.h>

#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

#include <fmt/format.h>

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>

#include <cfloat>
#include <cstring>

namespace Assets
{
    const unsigned int kImportFlags =
        aiProcess_CalcTangentSpace |
        aiProcess_GenSmoothNormals |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_ValidateDataStructure |
        aiProcess_GlobalScale |
        aiProcess_LimitBoneWeights |
        aiProcess_FlipUVs;

    float4x4 ToLinalg(const aiMatrix4x4& m)
    {
        return float4x4(
            float4(m.a1, m.b1, m.c1, m.d1),
            float4(m.a2, m.b2, m.c2, m.d2),
            float4(m.a3, m.b3, m.c3, m.d3),
            float4(m.a4, m.b4, m.c4, m.d4)
        );
    }

    float3 ToFloat3(const aiVector3D& v)
    {
        return float3(v.x, v.y, v.z);
    }

    bool IsIdentity(const aiMatrix4x4& m)
    {
        return m.a1 == 1.0f && m.a2 == 0.0f && m.a3 == 0.0f && m.a4 == 0.0f &&
               m.b1 == 0.0f && m.b2 == 1.0f && m.b3 == 0.0f && m.b4 == 0.0f &&
               m.c1 == 0.0f && m.c2 == 0.0f && m.c3 == 1.0f && m.c4 == 0.0f &&
               m.d1 == 0.0f && m.d2 == 0.0f && m.d3 == 0.0f && m.d4 == 1.0f;
    }

    // Collapse "." / ".." segments so cooked mirror paths stay clean (NormalizeAssetPath and
    // MakeAssetsRelative do not collapse interior ".."). A ".." that would climb above the
    // assets root is dropped (clamped at the root).
    eastl::string CollapseDots(const eastl::string& path)
    {
        eastl::vector<eastl::string> parts;
        size_t start = 0;
        while (start <= path.size())
        {
            const size_t sep = path.find('/', start);
            const size_t len = (sep == eastl::string::npos) ? eastl::string::npos : sep - start;
            const eastl::string seg = path.substr(start, len);
            if (seg == "..")
            {
                if (!parts.empty())
                    parts.pop_back();
            }
            else if (seg != "." && !seg.empty())
            {
                parts.push_back(seg);
            }
            if (sep == eastl::string::npos)
                break;
            start = sep + 1;
        }
        eastl::string result;
        for (size_t i = 0; i < parts.size(); i++)
        {
            if (i > 0)
                result += '/';
            result += parts[i];
        }
        return result;
    }

    void AppendTextureRef(FImportMaterial& mat, const aiMaterial* aiMat, aiTextureType type, ETextureUsage usage, bool bSRGB, const eastl::string& srcDir)
    {
        aiString aiPath;
        if (aiMat == nullptr || aiMat->GetTexture(type, 0, &aiPath) != aiReturn_SUCCESS)
            return;
        const char* raw = aiPath.C_Str();
        if (raw[0] == '\0')
            return;
        if (raw[0] == '*')
        {
            VTNA_LOG_WARN("[AssimpImporter] embedded texture {} cannot be cooked from disk; ref skipped", raw);
            return;
        }
        FAssetTextureRef texRef;
        texRef.Usage = usage;
        texRef.bSRGB = bSRGB;

        eastl::string rel = raw;
        for (char& c : rel)
        {
            if (c == '\\')
                c = '/';
        }

        const bool bAbsolute = !rel.empty() && (rel[0] == '/' || (rel.size() >= 2 && rel[1] == ':'));
        if (bAbsolute)
        {
            const size_t sep = rel.find_last_of('/');
            rel = (sep != eastl::string::npos) ? rel.substr(sep + 1) : rel;
        }
        texRef.RelativePath = FAssetPathUtils::MakeCookedTexturePath(CollapseDots(srcDir + rel), usage);
        mat.Textures.push_back(eastl::move(texRef));
    }

    void LoadMaterials(const aiScene* scene, const eastl::string& srcDir, FImportResult& out)
    {
        for (unsigned int i = 0; i < scene->mNumMaterials; i++)
        {
            const aiMaterial* aiMat = scene->mMaterials[i];
            FImportMaterial mat;
            aiString name;
            if (aiMat->Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS && name.length > 0)
                mat.Name = name.C_Str();
            else
                mat.Name = fmt::format("Material{}", i).c_str();

            // Only the metallic-roughness workflow is produced; OBJ/DAE have no PBR data,
            // FBX may, and a specular-glossiness-only material is converted to MR defaults.
            mat.WorkFlow = EMaterialWorkFlow::PBRMetallicRoughness;

            aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &baseColor) != AI_SUCCESS)
                aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &baseColor); // FBX/DAE/OBJ fallback
            mat.AlbedoColor = float3(baseColor.r, baseColor.g, baseColor.b);

            float metallic = 0.0f;
            float roughness = 1.0f; // legacy sources carry no MR data -> fully rough, not a perfect mirror (the FImportMaterial default of 0)

            aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &metallic);
            aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness);
            mat.Metallic = metallic;
            mat.Roughness = roughness;

            aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &mat.Metallic);
            aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &mat.Roughness);
            mat.Metallic = metallic;
            mat.Roughness = roughness;

            aiColor4D emissive(0.0f, 0.0f, 0.0f, 1.0f);
            aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &emissive);
            mat.EmissiveColor = float3(emissive.r, emissive.g, emissive.b);

            float opacity = 1.0f;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_OPACITY, &opacity) == AI_SUCCESS && opacity < 0.9999f)
                mat.bAlphaBlend = true;
            int twoSided = 0;
            if (aiGetMaterialInteger(aiMat, AI_MATKEY_TWOSIDED, &twoSided) == AI_SUCCESS && twoSided != 0)
                mat.bDoubleSided = true;

            // Texture slots: the fallback slot is only consulted when the primary is absent,
            // so a PBR FBX export does not also record the legacy diffuse map as BaseColor.
            const size_t baseCount = mat.Textures.size();
            AppendTextureRef(mat, aiMat, aiTextureType_BASE_COLOR, ETextureUsage::BaseColor, true, srcDir);
            if (mat.Textures.size() == baseCount)
                AppendTextureRef(mat, aiMat, aiTextureType_DIFFUSE, ETextureUsage::BaseColor, true, srcDir);
            AppendTextureRef(mat, aiMat, aiTextureType_NORMALS, ETextureUsage::Normal, false, srcDir);
            const size_t mrCount = mat.Textures.size();
            AppendTextureRef(mat, aiMat, aiTextureType_METALNESS, ETextureUsage::MetallicRoughness, false, srcDir);
            if (mat.Textures.size() == mrCount)
                AppendTextureRef(mat, aiMat, aiTextureType_DIFFUSE_ROUGHNESS, ETextureUsage::MetallicRoughness, false, srcDir);
            AppendTextureRef(mat, aiMat, aiTextureType_EMISSIVE, ETextureUsage::Emissive, true, srcDir);
            const size_t aoCount = mat.Textures.size();
            AppendTextureRef(mat, aiMat, aiTextureType_AMBIENT_OCCLUSION, ETextureUsage::AmbientOcclusion, false, srcDir);
            if (mat.Textures.size() == aoCount)
                AppendTextureRef(mat, aiMat, aiTextureType_LIGHTMAP, ETextureUsage::AmbientOcclusion, false, srcDir);

            out.Materials.push_back(eastl::move(mat));
        }
    }

    void LoadNodes(const aiScene* scene, FImportResult& out, eastl::hash_map<eastl::string, int32_t>& outNodeByName, eastl::vector<eastl::vector<uint32_t>>& outNodeMeshRefs, eastl::hash_set<uint32_t>& outReferencedMeshes)
    {
        struct FStackEntry
        {
            const aiNode* Node;
            int32_t Parent;
            float4x4 World;
        };
        eastl::vector<FStackEntry> stack;
        stack.push_back({scene->mRootNode, -1, float4x4({identity})});

        while (!stack.empty())
        {
            const FStackEntry entry = stack.back();
            stack.pop_back();
            const aiNode* node = entry.Node;

            const bool bInvisibleIdentityRoot = (node == scene->mRootNode) && node->mNumMeshes == 0 && IsIdentity(node->mTransformation);
            if (bInvisibleIdentityRoot)
            {
                for (unsigned int i = 0; i < node->mNumChildren; i++)
                {
                    const unsigned int child = node->mNumChildren - 1 - i;
                    stack.push_back({node->mChildren[child], -1, entry.World});
                }
                continue;
            }

            const int32_t nodeIndex = static_cast<int32_t>(out.Nodes.size());
            const float4x4 local = ToLinalg(node->mTransformation);
            const float4x4 world = entry.Parent < 0 ? local : mul(entry.World, local);

            FImportNode& outNode = out.Nodes.emplace_back();
            outNode.Name = node->mName.length > 0 ? node->mName.C_Str() : fmt::format("Node_{}", nodeIndex).c_str();
            outNode.Parent = entry.Parent;

            // RH->LH: mirror Z. bFrontFaceCCW uses the world determinant, computed pre-flip
            // (the determinant sign is invariant under the mirror, so it is equivalent).
            Decompose(local, outNode.Translation, outNode.Rotation, outNode.Scale);
            outNode.Translation.z = -outNode.Translation.z;
            outNode.Rotation.z = -outNode.Rotation.z;
            outNode.Rotation.w = -outNode.Rotation.w;
            outNode.bFrontFaceCCW = node->mNumMeshes > 0 && determinant(world) > 0.0f;

            // First-wins name map: bone / animation targets resolve to the earliest node.
            if (!outNode.Name.empty() && outNodeByName.find(outNode.Name) == outNodeByName.end())
                outNodeByName[outNode.Name] = nodeIndex;

            outNodeMeshRefs.emplace_back();
            for (unsigned int m = 0; m < node->mNumMeshes; m++)
            {
                const uint32_t meshIdx = node->mMeshes[m];
                if (meshIdx < scene->mNumMeshes)
                {
                    outNodeMeshRefs[nodeIndex].push_back(meshIdx);
                    outReferencedMeshes.insert(meshIdx);
                }
            }

            // Back edge: AssetValidate.cpp and the runtime hierarchy DFS (ModelLoader.cpp)
            // both require Parent/Children to be consistent. The index-based write is safe
            // across the emplace_back above (no dangling reference).
            if (entry.Parent >= 0)
                out.Nodes[static_cast<size_t>(entry.Parent)].Children.push_back(nodeIndex);
            else
                out.RootNodes.push_back(nodeIndex);

            // Children are pushed in reverse so the pre-order visit keeps the file order.
            for (unsigned int c = 0; c < node->mNumChildren; c++)
            {
                const unsigned int child = node->mNumChildren - 1 - c;
                stack.push_back({node->mChildren[child], nodeIndex, world});
            }
        }
    }

    void CollectJoints(const aiScene* scene, const eastl::hash_set<uint32_t>& referencedMeshes, const eastl::hash_map<eastl::string, int32_t>& nodeByName, eastl::vector<eastl::vector<int32_t>>& outMeshBoneRemap, FImportResult& out)
    {
        outMeshBoneRemap.resize(scene->mNumMeshes);
        for (uint32_t i = 0; i < scene->mNumMeshes; i++)
        {
            const aiMesh* mesh = scene->mMeshes[i];
            if (referencedMeshes.find(i) == referencedMeshes.end() || mesh->mNumBones == 0)
                continue;
            eastl::vector<int32_t>& remap = outMeshBoneRemap[i];
            remap.reserve(mesh->mNumBones);
            for (unsigned int b = 0; b < mesh->mNumBones; b++)
            {
                const aiBone* bone = mesh->mBones[b];
                const eastl::string boneName = bone->mName.C_Str();
                const auto nodeIt = nodeByName.find(boneName);
                if (nodeIt == nodeByName.end())
                {
                    VTNA_LOG_WARN("[AssimpImporter] bone {} of mesh {} has no matching node; weights skipped", boneName, i);
                    remap.push_back(-1);
                    continue;
                }
                int32_t joint = -1;
                for (size_t j = 0; j < out.Joints.size(); j++)
                {
                    if (out.Joints[j] == nodeIt->second)
                    {
                        joint = static_cast<int32_t>(j);
                        break;
                    }
                }
                if (joint < 0)
                {
                    joint = static_cast<int32_t>(out.Joints.size());
                    out.Joints.push_back(nodeIt->second);
                    // IBM flip, same convention as GLTFImporter::LoadSkeleton.
                    float3 t;
                    float4 r;
                    float3 s;
                    const float4x4 ibm = ToLinalg(bone->mOffsetMatrix);
                    Decompose(ibm, t, r, s);
                    t.z = -t.z;
                    r.z = -r.z;
                    r.w = -r.w;
                    const float4x4 T = translation_matrix(t);
                    const float4x4 R = rotation_matrix(r);
                    const float4x4 S = scaling_matrix(s);
                    out.InverseBindMatrices.push_back(mul(T, mul(R, S)));
                }
                remap.push_back(joint);
            }
        }
    }

    // Cook one aiMesh into a primitive. Joints/Weights are written in skeleton space via
    // boneRemap (mesh-local bone index -> global joint index, -1 = no node -> skipped).
    bool LoadMesh(const aiMesh* mesh, uint32_t meshIndex, unsigned int numMaterials, const eastl::vector<int32_t>& boneRemap, FImportResult& out)
    {
        if (mesh->mNumVertices == 0 || mesh->mNumFaces == 0 || mesh->mVertices == nullptr)
            return false;

        FImportPrimitive prim;
        prim.Name = fmt::format("Mesh_{} : {}", meshIndex, mesh->mName.length > 0 ? mesh->mName.C_Str() : "").c_str();
        // Scene with no materials (raw STL/PLY): -1 is FImportPrimitive's legal default and
        // passes validation; a clamped bogus index would fail the whole cook.
        prim.MaterialIndex = mesh->mMaterialIndex < numMaterials ? static_cast<int32_t>(mesh->mMaterialIndex) : -1;
        // A mesh whose bones all failed to resolve to a node (bone name absent from the
        // scene graph) would cook into an empty skeleton with all-zero Joints/Weights;
        // degrade it to a static mesh instead (CollectJoints logged the skips).
        bool bAnyValidJoint = false;
        for (int32_t j : boneRemap)
        {
            if (j >= 0)
            {
                bAnyValidJoint = true;
                break;
            }
        }
        prim.bSkeletal = mesh->mNumBones > 0 && bAnyValidJoint;
        prim.bSkeletalAnim = prim.bSkeletal;

        // ---- indices (Triangulate + SortByPType guarantee triangles; skip stragglers) ----
        eastl::vector<uint32_t> triangles;
        triangles.reserve(mesh->mNumFaces * 3);
        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3)
            {
                VTNA_LOG_WARN("[AssimpImporter] mesh {} has a non-triangle face ({} indices); face skipped", meshIndex, face.mNumIndices);
                continue;
            }
            for (unsigned int k = 0; k < 3; k++)
                triangles.push_back(face.mIndices[k]);
        }
        if (triangles.empty())
            return false;
        for (uint32_t idx : triangles)
        {
            if (idx >= mesh->mNumVertices)
            {
                VTNA_LOG_WARN("[AssimpImporter] mesh {} has an out-of-range index {} ({} vertices); mesh skipped", meshIndex, idx, mesh->mNumVertices);
                return false;
            }
        }
        prim.IndexCount = static_cast<uint32_t>(triangles.size());
        prim.IndexStride = mesh->mNumVertices <= 65535 ? 2 : 4;
        prim.IndexData.resize(static_cast<size_t>(triangles.size()) * prim.IndexStride);
        if (prim.IndexStride == 2)
        {
            uint16_t* dst = reinterpret_cast<uint16_t*>(prim.IndexData.data());
            for (size_t i = 0; i < triangles.size(); i++)
                dst[i] = static_cast<uint16_t>(triangles[i]);
        }
        else
        {
            memcpy(prim.IndexData.data(), triangles.data(), triangles.size() * sizeof(uint32_t));
        }
        prim.VertexCount = mesh->mNumVertices;

        // ---- position (RH -> LH) + bounding sphere from the flipped AABB ----
        FImportVertexStream posStream;
        posStream.Type = EVertexStreamType::Position;
        posStream.Stride = sizeof(float3);
        posStream.Data.resize(static_cast<size_t>(mesh->mNumVertices) * sizeof(float3));
        float3* dstPos = reinterpret_cast<float3*>(posStream.Data.data());
        float3 minV(FLT_MAX);
        float3 maxV(-FLT_MAX);
        for (unsigned int v = 0; v < mesh->mNumVertices; v++)
        {
            dstPos[v] = float3(mesh->mVertices[v].x, mesh->mVertices[v].y, -mesh->mVertices[v].z);
            minV = linalg::min(minV, dstPos[v]);
            maxV = linalg::max(maxV, dstPos[v]);
        }
        prim.Center = (minV + maxV) * 0.5f;
        prim.Radius = length(maxV - minV) * 0.5f;
        prim.Streams.push_back(eastl::move(posStream));

        // ---- texcoord (first UV set only, float2; known limitation, matches GLTFImporter) ----
        if (mesh->mTextureCoords[0] != nullptr && mesh->mNumUVComponents[0] > 0)
        {
            FImportVertexStream uvStream;
            uvStream.Type = EVertexStreamType::TexCoord;
            uvStream.Stride = sizeof(float2);
            uvStream.Data.resize(static_cast<size_t>(mesh->mNumVertices) * sizeof(float2));
            float2* dstUV = reinterpret_cast<float2*>(uvStream.Data.data());
            for (unsigned int v = 0; v < mesh->mNumVertices; v++)
                dstUV[v] = float2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            prim.Streams.push_back(eastl::move(uvStream));
        }

        // ---- normal + tangent ----
        if (mesh->mNormals != nullptr)
        {
            FImportVertexStream nStream;
            nStream.Type = EVertexStreamType::Normal;
            nStream.Stride = sizeof(float3);
            nStream.Data.resize(static_cast<size_t>(mesh->mNumVertices) * sizeof(float3));
            float3* dstN = reinterpret_cast<float3*>(nStream.Data.data());
            for (unsigned int v = 0; v < mesh->mNumVertices; v++)
                dstN[v] = float3(mesh->mNormals[v].x, mesh->mNormals[v].y, -mesh->mNormals[v].z);
            prim.Streams.push_back(eastl::move(nStream));

            if (mesh->mTangents != nullptr && mesh->mBitangents != nullptr)
            {
                FImportVertexStream tStream;
                tStream.Type = EVertexStreamType::Tangent;
                tStream.Stride = sizeof(float4);
                tStream.Data.resize(static_cast<size_t>(mesh->mNumVertices) * sizeof(float4));
                float4* dstT = reinterpret_cast<float4*>(tStream.Data.data());
                for (unsigned int v = 0; v < mesh->mNumVertices; v++)
                {
                    // Handedness is computed from the source (RH) data. The z-mirror NEGATES the
                    // scalar triple product (for S = diag(1,1,-1):
                    // dot(cross(Sa,Sb),Sc) == -dot(cross(a,b),c)), so the LH-correct sign
                    // would be -dot(cross(N,T),B). The source-space sign is kept to stay
                    // consistent with the glTF path (GLTFImporter stores w un-flipped too);
                    // the engine-side sign fix (Model.hlsl bitangent rebuild) is separate.
                    const float3 n = ToFloat3(mesh->mNormals[v]);
                    const float3 t = ToFloat3(mesh->mTangents[v]);
                    const float3 b = ToFloat3(mesh->mBitangents[v]);
                    dstT[v] = float4(t.x, t.y, -t.z, dot(cross(n, t), b) >= 0.0f ? 1.0f : -1.0f);
                }
                prim.Streams.push_back(eastl::move(tStream));
            }
        }

        // ---- joints / weights (skeleton-space indices, ushort4/float4 like glTF) ----
        if (prim.bSkeletal)
        {
            struct FBoneWeight
            {
                uint16_t Joint;
                float Weight;
            };
            struct FVertexBones
            {
                uint8_t Count = 0;
                FBoneWeight W[4];
            };
            // Flat per-vertex slots (LimitBoneWeights guarantees <= 4): a 100k-vertex
            // character costs zero per-vertex heap allocations (vector<vector> would
            // allocate and reallocate once per weighted vertex).
            eastl::vector<FVertexBones> perVertex(mesh->mNumVertices);
            for (unsigned int b = 0; b < mesh->mNumBones; b++)
            {
                const int32_t joint = b < boneRemap.size() ? boneRemap[b] : -1;
                if (joint < 0)
                    continue;
                const aiBone* bone = mesh->mBones[b];
                for (unsigned int w = 0; w < bone->mNumWeights; w++)
                {
                    const unsigned int vid = bone->mWeights[w].mVertexId;
                    if (vid >= mesh->mNumVertices)
                        continue;
                    const FBoneWeight wgt = {static_cast<uint16_t>(joint), bone->mWeights[w].mWeight};
                    FVertexBones& slot = perVertex[vid];
                    if (slot.Count < 4)
                    {
                        slot.W[slot.Count++] = wgt;
                    }
                    else
                    {
                        // Defensive: >4 weights (LimitBoneWeights enforces <= 4) — replace
                        // the lightest entry when the new one is heavier.
                        uint8_t lightest = 0;
                        for (uint8_t k = 1; k < 4; k++)
                        {
                            if (slot.W[k].Weight < slot.W[lightest].Weight)
                                lightest = k;
                        }
                        if (wgt.Weight > slot.W[lightest].Weight)
                            slot.W[lightest] = wgt;
                    }
                }
            }

            FImportVertexStream jStream;
            jStream.Type = EVertexStreamType::Joints;
            jStream.Stride = sizeof(uint16_t) * 4;
            jStream.Data.resize(static_cast<size_t>(mesh->mNumVertices) * jStream.Stride); // zero-filled
            FImportVertexStream wStream;
            wStream.Type = EVertexStreamType::Weights;
            wStream.Stride = sizeof(float) * 4;
            wStream.Data.resize(static_cast<size_t>(mesh->mNumVertices) * wStream.Stride); // zero-filled
            uint16_t* dstJ = reinterpret_cast<uint16_t*>(jStream.Data.data());
            float* dstW = reinterpret_cast<float*>(wStream.Data.data());
            for (unsigned int v = 0; v < mesh->mNumVertices; v++)
            {
                FVertexBones& slot = perVertex[v];
                // Insertion sort (<=4 entries) with a joint-index tie-break: equal weights
                // resolve deterministically across EASTL versions (eastl::sort is unstable).
                for (uint8_t k = 1; k < slot.Count; k++)
                {
                    const FBoneWeight tmp = slot.W[k];
                    uint8_t j = k;
                    while (j > 0 && (slot.W[j - 1].Weight < tmp.Weight ||
                                     (slot.W[j - 1].Weight == tmp.Weight && slot.W[j - 1].Joint > tmp.Joint)))
                    {
                        slot.W[j] = slot.W[j - 1];
                        --j;
                    }
                    slot.W[j] = tmp;
                }
                float sum = 0.0f;
                for (uint8_t k = 0; k < slot.Count; k++)
                    sum += slot.W[k].Weight;
                const float inv = sum > 0.0f ? 1.0f / sum : 0.0f;
                for (uint8_t k = 0; k < slot.Count; k++)
                {
                    dstJ[v * 4 + k] = slot.W[k].Joint;
                    dstW[v * 4 + k] = slot.W[k].Weight * inv;
                }
            }
            prim.Streams.push_back(eastl::move(jStream));
            prim.Streams.push_back(eastl::move(wStream));
        }

        out.Primitives.push_back(eastl::move(prim));
        return true;
    }

    void LoadMeshes(const aiScene* scene, const eastl::hash_set<uint32_t>& referencedMeshes, const eastl::vector<eastl::vector<int32_t>>& meshBoneRemap, eastl::hash_map<uint32_t, int32_t>& outMeshToPrim, FImportResult& out)
    {
        for (uint32_t i = 0; i < scene->mNumMeshes; i++)
        {
            if (referencedMeshes.find(i) == referencedMeshes.end())
                continue;
            if (LoadMesh(scene->mMeshes[i], i, scene->mNumMaterials, meshBoneRemap[i], out))
                outMeshToPrim[i] = static_cast<int32_t>(out.Primitives.size() - 1);
        }
    }

    void AttachPrimitives(eastl::vector<FImportNode>& nodes, const eastl::vector<eastl::vector<uint32_t>>& nodeMeshRefs, const eastl::hash_map<uint32_t, int32_t>& meshToPrim)
    {
        for (size_t i = 0; i < nodes.size(); i++)
        {
            for (uint32_t meshIdx : nodeMeshRefs[i])
            {
                const auto it = meshToPrim.find(meshIdx);
                if (it != meshToPrim.end())
                    nodes[i].Primitives.push_back(it->second);
            }
        }
    }

    // One vector-valued channel (mode 0=translation, 2=scale). Keyframe values are kept in
    // the source (RH) convention: the runtime flips them at sample time
    // (FAnimation::UpdateChannel), exactly like the glTF path.
    void AppendVectorChannel(FAssetAnimation& anim, int32_t targetNode, uint32_t mode, const aiVectorKey* keys, unsigned int count, double tps)
    {
        FAssetAnimationChannel channel;
        channel.TargetNode = targetNode;
        channel.Mode = mode;
        channel.KeyFrames.reserve(count);
        for (unsigned int k = 0; k < count; k++)
        {
            const float time = static_cast<float>(keys[k].mTime / tps);
            const aiVector3D& v = keys[k].mValue;
            channel.KeyFrames.push_back(eastl::make_pair(time, float4(v.x, v.y, v.z, 0.0f)));
        }
        if (channel.KeyFrames.empty())
            return;
        const float first = channel.KeyFrames.front().first;
        const float last = channel.KeyFrames.back().first;
        anim.TimeDuration = eastl::max(anim.TimeDuration, eastl::max(0.0f, last - first));
        anim.Channels.push_back(eastl::move(channel));
    }

    void AppendRotationChannel(FAssetAnimation& anim, int32_t targetNode, const aiQuatKey* keys, unsigned int count, double tps)
    {
        FAssetAnimationChannel channel;
        channel.TargetNode = targetNode;
        channel.Mode = 1;
        channel.KeyFrames.reserve(count);
        for (unsigned int k = 0; k < count; k++)
        {
            const float time = static_cast<float>(keys[k].mTime / tps);
            const aiQuaternion& q = keys[k].mValue; // (w, x, y, z) -> engine (x, y, z, w)
            channel.KeyFrames.push_back(eastl::make_pair(time, float4(q.x, q.y, q.z, q.w)));
        }
        if (channel.KeyFrames.empty())
            return;
        const float first = channel.KeyFrames.front().first;
        const float last = channel.KeyFrames.back().first;
        anim.TimeDuration = eastl::max(anim.TimeDuration, eastl::max(0.0f, last - first));
        anim.Channels.push_back(eastl::move(channel));
    }

    void LoadAnimations(const aiScene* scene, const eastl::hash_map<eastl::string, int32_t>& nodeByName, FImportResult& out)
    {
        for (unsigned int a = 0; a < scene->mNumAnimations; a++)
        {
            const aiAnimation* aiAnim = scene->mAnimations[a];
            FAssetAnimation anim;
            anim.Name = aiAnim->mName.length > 0 ? aiAnim->mName.C_Str() : fmt::format("Anim_{}", a).c_str();

            // Key times are in ticks; mTicksPerSecond == 0 means the file did not specify a
            // rate (assimp's convention then is 1 tick == 1 second). DAE stores ms with
            // tps = 1000 (ColladaLoader.cpp), FBX stores ticks with its own tps — both
            // collapse to seconds through the same division.
            const double tps = aiAnim->mTicksPerSecond > 0.0 ? aiAnim->mTicksPerSecond : 1.0;

            for (unsigned int c = 0; c < aiAnim->mNumChannels; c++)
            {
                const aiNodeAnim* nodeAnim = aiAnim->mChannels[c];
                const eastl::string nodeName = nodeAnim->mNodeName.C_Str();
                const auto nodeIt = nodeByName.find(nodeName);
                if (nodeIt == nodeByName.end())
                {
                    VTNA_LOG_WARN("[AssimpImporter] animation {} channel targets unknown node {}; skipped", anim.Name, nodeName);
                    continue;
                }
                const int32_t targetNode = nodeIt->second;
                if (nodeAnim->mNumPositionKeys > 0)
                    AppendVectorChannel(anim, targetNode, 0, nodeAnim->mPositionKeys, nodeAnim->mNumPositionKeys, tps);
                if (nodeAnim->mNumRotationKeys > 0)
                    AppendRotationChannel(anim, targetNode, nodeAnim->mRotationKeys, nodeAnim->mNumRotationKeys, tps);
                if (nodeAnim->mNumScalingKeys > 0)
                    AppendVectorChannel(anim, targetNode, 2, nodeAnim->mScalingKeys, nodeAnim->mNumScalingKeys, tps);
            }

            // Honor the declared clip duration (loop length) when it exceeds the key range
            // (mDuration defaults to -1 when the file declares none; then <= 0 == no-op).
            if (aiAnim->mDuration > 0.0)
                anim.TimeDuration = eastl::max(anim.TimeDuration, static_cast<float>(aiAnim->mDuration / tps));
            out.Animations.push_back(eastl::move(anim));
        }
    }

    bool FAssimpImporter::Import(const eastl::string& srcPath, FImportResult& out)
    {
        Assimp::Importer importer;
        // Unit scale: GlobalScale with factor 1.0 is a global no-op (it only scales by
        // factor * app scale); the FBX cm->m conversion is a separate switch that IS set
        // here (a no-op for OBJ/DAE). Without it, cm-authored FBX cooks 100x too large.
        importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0f);
        importer.SetPropertyBool(AI_CONFIG_FBX_CONVERT_TO_M, true);
        // Normals generated for sources without them (bare OBJ) must not blur hard edges:
        // 80 deg is the "decent smoothing angle" from the GenSmoothNormals docs.
        importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
        const aiScene* scene = importer.ReadFile(srcPath.c_str(), kImportFlags);
        if (scene == nullptr || scene->mRootNode == nullptr)
        {
            VTNA_LOG_ERROR("[AssimpImporter] ReadFile failed for {}: {}", srcPath, importer.GetErrorString());
            return false;
        }
        if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0)
        {
            VTNA_LOG_ERROR("[AssimpImporter] {} imported incompletely ({}); refusing to cook", srcPath, importer.GetErrorString());
            return false;
        }

        // Texture refs and headers resolve against the source dir, Assets-relative
        // (same convention as FGLTFImporter::Import).
        const size_t lastSlash = srcPath.find_last_of("/\\");
        const eastl::string rawSrcDir = (lastSlash == eastl::string::npos) ? "" : srcPath.substr(0, lastSlash + 1);
        const eastl::string srcDir = FAssetPathUtils::MakeAssetsRelative(rawSrcDir, FAssetPathUtils::GetAssetsRootPath());

        // Stage order mirrors FGLTFImporter: materials -> nodes -> roots -> meshes -> skeleton -> animations.
        eastl::hash_map<eastl::string, int32_t> nodeByName;
        eastl::hash_set<uint32_t> referencedMeshes;
        eastl::vector<eastl::vector<uint32_t>> nodeMeshRefs;
        LoadNodes(scene, out, nodeByName, nodeMeshRefs, referencedMeshes);
        LoadMaterials(scene, srcDir, out);
        eastl::vector<eastl::vector<int32_t>> meshBoneRemap;
        CollectJoints(scene, referencedMeshes, nodeByName, meshBoneRemap, out);
        eastl::hash_map<uint32_t, int32_t> meshToPrim;
        LoadMeshes(scene, referencedMeshes, meshBoneRemap, meshToPrim, out);
        AttachPrimitives(out.Nodes, nodeMeshRefs, meshToPrim);
        LoadAnimations(scene, nodeByName, out);
        return true;
    }
}
