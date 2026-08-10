#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetCooker/GLTFImporter.hpp"

#include <cgltf/cgltf.h>

#include "Utilities/Math.hpp"
#include "Utilities/Log.hpp"

#include <fmt/format.h>

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>

#include <cassert>
#include <cstring>

namespace 
{
    inline bool IsFrontFaceCCW(const cgltf_node* node)
    {
        cgltf_float m[16];
        cgltf_node_transform_world(node, m);

        float4x4 mtx(m);
        return determinant(mtx) > 0.0f;
    }

    // Resolve the raw byte range of an accessor's data with a bounds check against the
    // containing buffer. Returns nullptr (and logs) when the accessor has no buffer view
    // (legal per glTF: data is all zeros) or the range falls outside the buffer.
    const uint8_t* GetAccessorData(const cgltf_accessor* accessor, size_t& outByteCount)
    {
        if (accessor == nullptr || accessor->buffer_view == nullptr || accessor->buffer_view->buffer == nullptr) 
            return nullptr;
        const cgltf_buffer* buffer = accessor->buffer_view->buffer;
        // Stride * count can itself overflow uint64 on hostile values; guard before widening
        // (a zero stride is rejected by LoadBufferStream before this is reached).
        if (accessor->stride != 0 && accessor->count > SIZE_MAX / accessor->stride)
        {
            VTNA_LOG_WARN("[GLTFImporter] accessor stride {} * count {} overflows", accessor->stride, accessor->count);
            return nullptr;
        }
        const uint64_t start = static_cast<uint64_t>(accessor->buffer_view->offset) + accessor->offset;
        const uint64_t byteCount = static_cast<uint64_t>(accessor->stride) * accessor->count;
        if (start + byteCount > buffer->size)
        {
            VTNA_LOG_WARN("[GLTFImporter] accessor range out of bounds ({} + {} > {}B buffer)", start, byteCount, buffer->size);
            return nullptr;
        }
        outByteCount = static_cast<size_t>(byteCount);
        return reinterpret_cast<const uint8_t*>(buffer->data) + start;
    }

    // Copy an accessor's bytes into a vertex stream; optionally flip Z (RH->LH).
    // Rejects sparse and interleaved accessors (a cooker must not silently bake wrong bytes).
    bool LoadBufferStream(const cgltf_accessor* accessor, bool convertToLH, Assets::FImportVertexStream& outStream)
    {
        if (accessor == nullptr)
            return false;
        // Sparse accessors need cgltf_accessor_unpack_floats; interleaved buffer views
        // (stride > component sum) would copy neighboring attribute bytes. Reject both.
        if (accessor->is_sparse)
        {
            VTNA_LOG_WARN("[GLTFImporter] sparse accessor not supported; skipping stream");
            return false;
        }
        const uint32_t stride = static_cast<uint32_t>(accessor->stride);
        // Element size = components × component size (e.g. float3 = 12B). Interleaved buffer
        // views have stride > this; reject them instead of baking neighboring bytes.
        const uint32_t elementSize = static_cast<uint32_t>(cgltf_calc_size(accessor->type, accessor->component_type));
        if (stride == 0 || stride > elementSize)
        {
            // stride == 0 happens for invalid type/component_type (cgltf_calc_size returns 0);
            // the old assert(stride >= sizeof(float3)) was Debug-only and wrote a null pointer
            // in Release. An offline tool must reject, not crash.
            VTNA_LOG_WARN("[GLTFImporter] accessor stride {} unsupported (element {}B); skipping stream", stride, elementSize);
            return false;
        }
        if (convertToLH && stride < sizeof(float3))
        {
            VTNA_LOG_WARN("[GLTFImporter] position stream stride {} < 12B; skipping", stride);
            return false;
        }
        size_t byteCount = 0;
        const uint8_t* src = GetAccessorData(accessor, byteCount);
        if (src == nullptr)
            return false;

        outStream.Stride = stride;
        outStream.Data.assign(src, src + byteCount);

        if (convertToLH)
        {
            for (cgltf_size i = 0; i < accessor->count; ++i)
            {
                float3& v = *reinterpret_cast<float3*>(outStream.Data.data() + stride * i);
                v.z = -v.z;
            }
        }
        return true;
    }

    // Migrated from FModelLoader::LoadMaterial, but only records texture *output paths*.
    Assets::FImportMaterial LoadImportMaterial(const cgltf_material* gltfMaterial)
    {
        Assets::FImportMaterial material;
        if (gltfMaterial == nullptr)
        {
            return material;
        }

        material.Name = gltfMaterial->name ? gltfMaterial->name : "";

        if (gltfMaterial->has_pbr_metallic_roughness)
        {
            material.WorkFlow = Assets::EMaterialWorkFlow::PBRMetallicRoughness;
            material.AlbedoColor = float3(gltfMaterial->pbr_metallic_roughness.base_color_factor);
            material.Metallic = gltfMaterial->pbr_metallic_roughness.metallic_factor;
            material.Roughness = gltfMaterial->pbr_metallic_roughness.roughness_factor;
        }
        else if (gltfMaterial->has_pbr_specular_glossiness)
        {
            material.WorkFlow = Assets::EMaterialWorkFlow::PBRSpecularGlossiness;
            material.DiffuseColor = float3(gltfMaterial->pbr_specular_glossiness.diffuse_factor);
            material.SpecularColor = float3(gltfMaterial->pbr_specular_glossiness.specular_factor);
            material.Glossiness = gltfMaterial->pbr_specular_glossiness.glossiness_factor;
        }
        material.EmissiveColor = float3(gltfMaterial->emissive_factor);
        material.AlphaCutout = gltfMaterial->alpha_cutoff;
        material.bAlphaTest = gltfMaterial->alpha_mode == cgltf_alpha_mode_mask;
        material.bAlphaBlend = gltfMaterial->alpha_mode == cgltf_alpha_mode_blend;
        material.bDoubleSided = gltfMaterial->double_sided;
        return material;
    }

    // Append a texture output-path reference (B5: "<uri>.<Usage>.vasset").
    // Handles external URI, embedded buffer-view image, and data URI.
    void AppendTextureRef(Assets::FImportMaterial& mat, const cgltf_texture_view& view, Assets::ETextureUsage usage, bool bSRGB, const eastl::string& srcDir)
    {
        const cgltf_image* image = view.texture ? view.texture->image : nullptr;
        if (image == nullptr)
        {
            return;
        }
        Assets::FAssetTextureRef ref;
        ref.Usage = usage;
        ref.bSRGB = bSRGB;
        if (view.has_transform)
        {
            ref.bHasTransform = true;
            ref.Offset = float2(view.transform.offset);
            ref.Scale = float2(view.transform.scale);
            ref.Rotation = static_cast<float>(view.transform.rotation);
        }

        if (image->uri == nullptr)
        {
            // Embedded image (buffer_view): synthesize a stable name so the ref is still
            // recorded and Slice 5 can cook it out of the buffer view.
            ref.RelativePath = Assets::FAssetPathUtils::MakeCookedTexturePath(srcDir + (image->name ? image->name : "embedded"), usage);
        }
        else if (strncmp(image->uri, "data:", 5) == 0)
        {
            ref.RelativePath = Assets::FAssetPathUtils::MakeCookedTexturePath(srcDir + (image->name ? image->name : "inline"), usage);
        }
        else
        {
            eastl::string uri = image->uri;
            cgltf_decode_uri(uri.begin());              // in-place percent-decode (%20 etc.)
            uri.resize(strlen(uri.c_str()));
            ref.RelativePath = Assets::FAssetPathUtils::MakeCookedTexturePath(srcDir + uri, usage);
        }
        mat.Textures.push_back(ref);
    }
} // namespace


namespace Assets
{
    bool FGLTFImporter::Import(const eastl::string& srcPath, FImportResult& out)
    {
        cgltf_options options = {};
        cgltf_data* data = nullptr;
        const cgltf_result parseResult = cgltf_parse_file(&options, srcPath.c_str(), &data);
        if (parseResult != cgltf_result_success)
        {
            VTNA_LOG_ERROR("[GLTFImporter] cgltf_parse_file failed ({}) for {}", static_cast<int>(parseResult), srcPath);
            return false;
        }
        const cgltf_result loadResult = cgltf_load_buffers(&options, data, srcPath.c_str());
        if (loadResult != cgltf_result_success)
        {
            VTNA_LOG_ERROR("[GLTFImporter] cgltf_load_buffers failed ({}) for {}", static_cast<int>(loadResult), srcPath);
            cgltf_free(data);
            return false;
        }
        if (cgltf_validate(data) != cgltf_result_success)
        {
            VTNA_LOG_ERROR("[GLTFImporter] cgltf_validate failed for {}; refusing to cook", srcPath);
            cgltf_free(data);
            return false;
        }

        const size_t lastSlash = srcPath.find_last_of('/');
        const eastl::string rawSrcDir = (lastSlash == eastl::string::npos) ? "" : srcPath.substr(0, lastSlash + 1);
        // A: normalize to AssetsPath-relative ("../Assets/Models/Sponza/glTF/" -> "Models/Sponza/glTF/").
        // The CLI and engine both resolve sources under "../Assets/"; the assets root is
        // intentionally not a parameter here (Slice 3 keeps Import's signature untouched).
        const eastl::string srcDir = FAssetPathUtils::MakeAssetsRelative(rawSrcDir, FAssetPathUtils::GetAssetsRootPath());

        // One pass over the parsed data, one stage per FImportResult section.
        LoadMaterials(data, srcDir, out);
        LoadNodes(data, out);
        const eastl::vector<const cgltf_node*> roots = LoadRootNodes(data, out);
        LoadMeshPrimitives(data, roots, out);
        LoadSkeleton(data, out);
        LoadAnimations(data, out);

        cgltf_free(data);
        return true;
    }

    void FGLTFImporter::LoadMaterials(cgltf_data* data, const eastl::string& srcDir, FImportResult& out)
    {
        // ---- Materials (cgltf guarantees data->materials are unique; no remap needed) ----
        for (cgltf_size i = 0; i < data->materials_count; i++)
        {
            FImportMaterial mat = LoadImportMaterial(&data->materials[i]);

            // cgltf_texture_view is a single value field (not an array); AppendTextureRef
            // tolerates a null .texture and a null image->uri (returns early / synthesizes).
            AppendTextureRef(mat, data->materials[i].pbr_metallic_roughness.base_color_texture, ETextureUsage::BaseColor, true, srcDir);
            AppendTextureRef(mat, data->materials[i].pbr_metallic_roughness.metallic_roughness_texture, ETextureUsage::MetallicRoughness, false, srcDir);
            if (data->materials[i].has_pbr_specular_glossiness)
            {
                AppendTextureRef(mat, data->materials[i].pbr_specular_glossiness.diffuse_texture, ETextureUsage::Diffuse, true, srcDir);
                AppendTextureRef(mat, data->materials[i].pbr_specular_glossiness.specular_glossiness_texture, ETextureUsage::SpecularGlossiness, false, srcDir);
            }
            AppendTextureRef(mat, data->materials[i].normal_texture, ETextureUsage::Normal, false, srcDir);
            AppendTextureRef(mat, data->materials[i].emissive_texture, ETextureUsage::Emissive, true, srcDir);
            AppendTextureRef(mat, data->materials[i].occlusion_texture, ETextureUsage::AmbientOcclusion, false, srcDir);

            out.Materials.push_back(eastl::move(mat));
        }
    }

    void FGLTFImporter::LoadNodes(cgltf_data* data, FImportResult& out)
    {
        // ---- Nodes (unified walk; per-node bSkeletal from node->skin) ----
        // Mirrors LoadSkeletalMeshNode: take TRS directly when the node has no matrix, so the
        // common path avoids a lossy decompose/recompose round trip (Decompose() also forces
        // scale >= 0, which would silently drop mirroring).
        out.Nodes.resize(data->nodes_count);
        for (cgltf_size i = 0; i < data->nodes_count; i++)
        {
            const cgltf_node* node = &data->nodes[i];
            out.Nodes[i].Name = node->name ? node->name : fmt::format("Node_{}", i).c_str();
            out.Nodes[i].Parent = node->parent ? static_cast<int32_t>(cgltf_node_index(data, node->parent)) : -1;
            for (cgltf_size c = 0; c < node->children_count; c++)
            {
                out.Nodes[i].Children.push_back(static_cast<int32_t>(cgltf_node_index(data, node->children[c])));
            }

            if (node->has_matrix)
            {
                Decompose(float4x4(node->matrix), out.Nodes[i].Translation, out.Nodes[i].Rotation, out.Nodes[i].Scale);
            }
            else
            {
                out.Nodes[i].Translation = float3(node->translation);
                out.Nodes[i].Rotation = float4(node->rotation);
                out.Nodes[i].Scale = float3(node->scale);
            }
            out.Nodes[i].Translation.z *= -1;
            out.Nodes[i].Rotation.z *= -1;
            out.Nodes[i].Rotation.w *= -1;
        }
    }

    eastl::vector<const cgltf_node*> FGLTFImporter::LoadRootNodes(cgltf_data* data, FImportResult& out)
    {
        // ---- Root nodes ----
        eastl::vector<const cgltf_node*> roots;
        if (data->scene != nullptr)
        {
            for (cgltf_size i = 0; i < data->scene->nodes_count; i++)
                roots.push_back(data->scene->nodes[i]);
        }
        else if (data->scenes_count > 0)
        {
            // No default scene: fall back to all scenes (review B5).
            for (cgltf_size s = 0; s < data->scenes_count; s++)
                for (cgltf_size i = 0; i < data->scenes[s].nodes_count; i++)
                    roots.push_back(data->scenes[s].nodes[i]);
        }
        else
        {
            // No scene at all: parentless nodes.
            for (cgltf_size i = 0; i < data->nodes_count; i++)
                if (data->nodes[i].parent == nullptr)
                    roots.push_back(&data->nodes[i]);
        }
        for (const cgltf_node* root : roots)
        {
            out.RootNodes.push_back(static_cast<int32_t>(cgltf_node_index(data, root)));
        }
        return roots;
    }

    void FGLTFImporter::LoadMeshPrimitives(cgltf_data* data, const eastl::vector<const cgltf_node*>& roots, FImportResult& out)
    {
        // ---- Mesh-bearing nodes -> primitives ----
        // One cgltf_mesh cooks to one range of primitives, shared by every node that references
        // it (this is why FAssetNode stores Primitives indices rather than inlining mesh data).
        eastl::hash_map<const cgltf_mesh*, eastl::vector<int32_t>> meshPrimCache;
        for (const cgltf_node* root : roots)
        {
            eastl::vector<const cgltf_node*> stack;
            stack.push_back(root);
            while (!stack.empty())
            {
                const cgltf_node* node = stack.back();
                stack.pop_back();

                if (node->mesh)
                {
                    const uint32_t meshIndex = static_cast<uint32_t>(cgltf_mesh_index(data, node->mesh));
                    const bool bSkeletal = node->skin != nullptr;
                    const bool bSkeletalAnim = bSkeletal;
                    const bool bFrontFaceCCW = IsFrontFaceCCW(node);

                    auto cacheIt = meshPrimCache.find(node->mesh);
                    if (cacheIt == meshPrimCache.end())
                    {
                        eastl::vector<int32_t>& primIndices = meshPrimCache[node->mesh];
                        for (cgltf_size p = 0; p < node->mesh->primitives_count; p++)
                        {
                            const cgltf_primitive* primitive = &node->mesh->primitives[p];

                            // Only indexed triangle lists are cooked (matches the meshlet pipeline);
                            // skip points/lines/non-indexed instead of failing the whole model.
                            if (primitive->type != cgltf_primitive_type_triangles || primitive->indices == nullptr)
                            {
                                VTNA_LOG_WARN("[GLTFImporter] skipping primitive {} of mesh {} (type={} indexed={})", p, meshIndex, static_cast<int>(primitive->type), primitive->indices != nullptr);
                                continue;
                            }

                            FImportPrimitive prim;
                            prim.Name = fmt::format("Mesh_{}_{} : {}", meshIndex, p, (node->mesh->name ? node->mesh->name : "")).c_str();
                            prim.MaterialIndex = primitive->material ? static_cast<int32_t>(cgltf_material_index(data, primitive->material)) : -1;
                            prim.bSkeletal = bSkeletal;
                            prim.bSkeletalAnim = bSkeletalAnim;

                            // --- indices ---
                            prim.IndexStride = static_cast<uint32_t>(primitive->indices->stride);
                            FImportVertexStream idxStream;
                            LoadBufferStream(primitive->indices, false, idxStream);
                            prim.IndexData = eastl::move(idxStream.Data);
                            prim.IndexCount = static_cast<uint32_t>(primitive->indices->count);

                            // --- vertex streams ---
                            for (cgltf_size a = 0; a < primitive->attributes_count; a++)
                            {
                                const cgltf_attribute& attr = primitive->attributes[a];
                                FImportVertexStream stream;
                                stream.Type = EVertexStreamType::Position;
                                bool convertToLH = false;
                                switch (attr.type)
                                {
                                case cgltf_attribute_type_position:
                                    stream.Type = EVertexStreamType::Position;
                                    convertToLH = true;
                                    break;
                                case cgltf_attribute_type_texcoord:
                                    if (attr.index != 0) continue; // only first UV set (known limitation, see §13)
                                    stream.Type = EVertexStreamType::TexCoord;
                                    break;
                                case cgltf_attribute_type_normal:
                                    stream.Type = EVertexStreamType::Normal;
                                    convertToLH = true;
                                    break;
                                case cgltf_attribute_type_tangent:
                                    stream.Type = EVertexStreamType::Tangent;
                                    convertToLH = true; // intentional fix: old skeletal path did not flip tangent
                                    break;
                                case cgltf_attribute_type_joints:
                                {
                                    // JOINTS_0 normalized to fixed ushort4 (8B/vertex), regardless of the
                                    // source component type (u8/u16): the GPU skinning path expects ushort4.
                                    stream.Type = EVertexStreamType::Joints;
                                    stream.Stride = static_cast<uint32_t>(sizeof(uint16_t) * 4);
                                    stream.Data.resize(stream.Stride * attr.data->count);
                                    uint16_t* dst = reinterpret_cast<uint16_t*>(stream.Data.data());
                                    for (cgltf_size j = 0; j < attr.data->count; j++)
                                    {
                                        cgltf_uint id[4] = {};
                                        cgltf_accessor_read_uint(attr.data, j, id, 4);
                                        for (int k = 0; k < 4; k++)
                                            dst[j * 4 + k] = static_cast<uint16_t>(id[k]);
                                    }
                                    prim.Streams.push_back(eastl::move(stream));
                                    continue;
                                }
                                case cgltf_attribute_type_weights:
                                {
                                    stream.Type = EVertexStreamType::Weights;
                                    stream.Stride = static_cast<uint32_t>(sizeof(float) * 4);
                                    stream.Data.resize(stream.Stride * attr.data->count);
                                    float* dst = reinterpret_cast<float*>(stream.Data.data());
                                    for (cgltf_size j = 0; j < attr.data->count; j++)
                                        cgltf_accessor_read_float(attr.data, j, dst + j * 4, 4);
                                    prim.Streams.push_back(eastl::move(stream));
                                    continue;
                                }
                                default:
                                    continue;
                                }
                                if (!LoadBufferStream(attr.data, convertToLH, stream))
                                {
                                    continue;
                                }
                                if (attr.type == cgltf_attribute_type_position)
                                {
                                    prim.VertexCount = static_cast<uint32_t>(attr.data->count);
                                    // Bounding sphere from accessor min/max (RH->LH flip on z)
                                    float3 minV = float3(attr.data->min);
                                    minV.z = -minV.z;
                                    float3 maxV = float3(attr.data->max);
                                    maxV.z = -maxV.z;
                                    prim.Center = (minV + maxV) * 0.5f;
                                    prim.Radius = length(maxV - minV) * 0.5f;
                                }
                                prim.Streams.push_back(eastl::move(stream));
                            }

                            const int32_t primIndex = static_cast<int32_t>(out.Primitives.size());
                            out.Primitives.push_back(eastl::move(prim));
                            primIndices.push_back(primIndex);
                        }
                        cacheIt = meshPrimCache.find(node->mesh);
                    }
                    // bFrontFaceCCW is a per-node property (a mirrored node transform flips triangle
                    // winding), so it is stored on FAssetNode, not the shared material.
                    out.Nodes[cgltf_node_index(data, node)].bFrontFaceCCW = bFrontFaceCCW;
                    out.Nodes[cgltf_node_index(data, node)].Primitives = cacheIt->second;
                }

                for (cgltf_size c = 0; c < node->children_count; c++)
                {
                    stack.push_back(node->children[c]);
                }
            }
        }
    }

    void FGLTFImporter::LoadSkeleton(cgltf_data* data, FImportResult& out)
    {
        // ---- Skeleton (first skin; matches old LoadSkeleton(data, &data->skins[0])) ----
        if (data->skins_count > 0)
        {
            const cgltf_skin* skin = &data->skins[0];
            for (cgltf_size i = 0; i < skin->joints_count; i++)
            {
                out.Joints.push_back(static_cast<int32_t>(cgltf_node_index(data, skin->joints[i])));
            }
            const cgltf_accessor* accessor = skin->inverse_bind_matrices;
            if (accessor != nullptr)
            {
                const bool bValidIBM = accessor->type == cgltf_type_mat4
                    && accessor->component_type == cgltf_component_type_r_32f
                    && accessor->stride >= sizeof(float4x4)
                    && accessor->count == skin->joints_count;

                if (!bValidIBM)
                {
                    VTNA_LOG_WARN("[GLTFImporter] inverse_bind_matrices layout unsupported (type={} comp={} stride={} count={} joints={}); falling back to identity",
                                  static_cast<int>(accessor->type), static_cast<int>(accessor->component_type),
                                  accessor->stride, accessor->count, skin->joints_count);
                    out.InverseBindMatrices.resize(skin->joints_count, float4x4({identity}));
                }
                else
                {
                    size_t byteCount = 0;
                    const uint8_t* src = GetAccessorData(accessor, byteCount);
                    if (src == nullptr)
                    {
                        out.InverseBindMatrices.resize(skin->joints_count, float4x4({identity}));
                    }
                    else
                    {
                        out.InverseBindMatrices.resize(accessor->count);
                        // Per-entry copy honoring a possible interleaved stride: never memcpy
                        // stride*count bytes into a 64B/count buffer.
                        for (cgltf_size i = 0; i < accessor->count; i++)
                        {
                            memcpy(&out.InverseBindMatrices[i], src + accessor->stride * i, sizeof(float4x4));
                        }
                        for (cgltf_size i = 0; i < accessor->count; i++)
                        {
                            float3 translation;
                            float4 rotation;
                            float3 scale;
                            Decompose(out.InverseBindMatrices[i], translation, rotation, scale);
                            translation.z = -translation.z;
                            rotation.z = -rotation.z;
                            rotation.w = -rotation.w;
                            float4x4 T = translation_matrix(translation);
                            float4x4 R = rotation_matrix(rotation);
                            float4x4 S = scaling_matrix(scale);
                            out.InverseBindMatrices[i] = mul(T, mul(R, S));
                        }
                    }
                }
            }
            else
            {
                out.InverseBindMatrices.resize(skin->joints_count, float4x4({identity}));
            }
        }
    }

    void FGLTFImporter::LoadAnimations(cgltf_data* data, FImportResult& out)
    {
        // ---- Animations ----
        for (cgltf_size i = 0; i < data->animations_count; i++)
        {
            const cgltf_animation* gltfAnim = &data->animations[i];
            FAssetAnimation anim;
            anim.Name = gltfAnim->name ? gltfAnim->name : "";
            for (cgltf_size c = 0; c < gltfAnim->channels_count; c++)
            {
                const cgltf_animation_channel* gltfChannel = &gltfAnim->channels[c];
                if (gltfChannel->target_node == nullptr)
                {
                    continue; // channel without a target is legal glTF
                }
                if (gltfChannel->target_path != cgltf_animation_path_type_translation &&
                    gltfChannel->target_path != cgltf_animation_path_type_rotation &&
                    gltfChannel->target_path != cgltf_animation_path_type_scale)
                {
                    continue; // morph weights etc. are not supported
                }
                const cgltf_animation_sampler* animSampler = gltfChannel->sampler;
                if (animSampler == nullptr || animSampler->interpolation != cgltf_interpolation_type_linear)
                {
                    VTNA_LOG_WARN("[GLTFImporter] channel {} uses non-LINEAR interpolation; skipped", c);
                    continue;
                }
                const cgltf_accessor* timeAccessor = animSampler->input;
                const cgltf_accessor* valueAccessor = animSampler->output;
                if (timeAccessor == nullptr || valueAccessor == nullptr || timeAccessor->count != valueAccessor->count)
                {
                    VTNA_LOG_WARN("[GLTFImporter] channel {} has mismatched keyframe counts; skipped", c);
                    continue;
                }

                FAssetAnimationChannel channel;
                channel.TargetNode = static_cast<int32_t>(cgltf_node_index(data, gltfChannel->target_node));
                switch (gltfChannel->target_path)
                {
                case cgltf_animation_path_type_translation:
                    channel.Mode = 0;
                    break;
                case cgltf_animation_path_type_rotation:
                    channel.Mode = 1;
                    break;
                case cgltf_animation_path_type_scale:
                    channel.Mode = 2;
                    break;
                default:
                    break;
                }

                size_t timeBytes = 0, valueBytes = 0;
                const uint8_t* timeData = GetAccessorData(timeAccessor, timeBytes);
                const uint8_t* valueData = GetAccessorData(valueAccessor, valueBytes);
                if (timeData == nullptr || valueData == nullptr)
                {
                    VTNA_LOG_WARN("[GLTFImporter] channel {} accessor has no buffer view; skipped", c);
                    continue;
                }
                for (cgltf_size k = 0; k < timeAccessor->count; k++)
                {
                    float time = 0.0f;
                    float4 value = float4(0.0f);
                    memcpy(&time, timeData + timeAccessor->stride * k, eastl::min<size_t>(timeAccessor->stride, sizeof(float)));
                    memcpy(&value, valueData + valueAccessor->stride * k, eastl::min<size_t>(valueAccessor->stride, sizeof(float4)));
                    channel.KeyFrames.push_back(eastl::make_pair(time, value));
                }

                if (channel.KeyFrames.empty())
                {
                    VTNA_LOG_WARN("[GLTFImporter] channel {} produced no keyframes; skipped", c);
                    continue;
                }

                // TimeDuration is the wrap point FAnimation::Update relies on to loop the
                // play head; a 0 duration would grow m_CurrentAnimTime unbounded and freeze
                // at the first frame after one pass. Accumulate the max over channels.
                const float first = channel.KeyFrames.front().first;
                const float last = channel.KeyFrames.back().first;
                anim.TimeDuration = eastl::max(anim.TimeDuration, eastl::max(0.0f, last - first));
                anim.Channels.push_back(eastl::move(channel));
            }
            out.Animations.push_back(eastl::move(anim));
        }
    }
} // namespace Assets
