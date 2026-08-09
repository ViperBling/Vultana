#include "AssetCooker/VultanaCooker.hpp"
#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetCooker/AssetValidate.hpp"
#include "AssetCooker/AssetWriter.hpp"
#include "AssetCooker/Importer.hpp"
#include "AssetCooker/GLTFImporter.hpp"
#include "AssetCooker/MeshCooker.hpp"

#include "Utilities/Log.hpp"

#include <EASTL/vector.h>
#include <EASTL/hash_set.h>

#include <sys/stat.h>
#include <filesystem>

namespace
{
    // eastl::string NormalizeSourcePath(const eastl::string& srcPath)
    // {
    //     const eastl::string prefix = "../Assets/";
    //     if (srcPath.compare(0, prefix.size(), prefix) == 0)
    //     {
    //         return srcPath.substr(prefix.size());
    //     }
    //     return srcPath;
    // }
}

namespace Assets
{
    bool Assets::FVultanaCooker::CookModel(const eastl::string& srcPath, const eastl::string& dstPath, bool bSilent, const eastl::string& assetsPath)
    {
        FGLTFImporter importer;
        FImportResult result;
        if (!importer.Import(srcPath, result))
        {
            if (!bSilent)
            {
                VTNA_LOG_ERROR("[Cooker] Failed to import model: {}", srcPath);
            }
            return false;
        }

        FModelCookedData payload;
        payload.Meshes.reserve(result.Primitives.size());
        uint32_t skippedMeshes = 0;

        eastl::vector<int32_t> primRemap(result.Primitives.size(), -1);
        for (size_t i = 0; i < result.Primitives.size(); i++)
        {
            const FImportPrimitive& prim = result.Primitives[i];
            FMeshCookedData mesh;
            if (!FMeshCooker::Cook(prim, mesh))
            {
                ++skippedMeshes;
                if (!bSilent) VTNA_LOG_WARN("[Cooker] Skipped mesh {}", prim.Name);
                continue;
            }
            primRemap[i] = static_cast<int32_t>(payload.Meshes.size());
            payload.Meshes.push_back(eastl::move(mesh));
        }
        if (payload.Meshes.empty())
        {
            if (!bSilent) VTNA_LOG_ERROR("[Cooker] No meshes cooked for {}", srcPath);
            return false;
        }

        // Rewrite node.Primitives into the payload.Meshes index space (drop skipped ones).
        for (FAssetNode& node : result.Nodes)
        {
            eastl::vector<int32_t> mappedPrims;
            mappedPrims.reserve(node.Primitives.size());
            for (int32_t oldIdx : node.Primitives)
            {
                if (oldIdx >= 0 && oldIdx < static_cast<int32_t>(primRemap.size()) && primRemap[oldIdx] >= 0)
                {
                    mappedPrims.push_back(primRemap[oldIdx]);
                }
            }
            node.Primitives = eastl::move(mappedPrims);
        }

        // Nodes / roots / materials map 1:1 from the import article
        payload.Nodes = eastl::move(result.Nodes);
        payload.RootNodes = eastl::move(result.RootNodes);
        payload.Materials.reserve(result.Materials.size());
        for (const FImportMaterial& mat : result.Materials)
        {
            FAssetMaterial material;
            material.Name = mat.Name;
            material.WorkFlow = mat.WorkFlow;
            material.Textures = mat.Textures;
            material.AlbedoColor = mat.AlbedoColor;
            material.Metallic = mat.Metallic;
            material.Roughness = mat.Roughness;
            material.DiffuseColor = mat.DiffuseColor;
            material.SpecularColor = mat.SpecularColor;
            material.Glossiness = mat.Glossiness;
            material.EmissiveColor = mat.EmissiveColor;
            material.AlphaCutout = mat.AlphaCutout;
            material.bAlphaTest = mat.bAlphaTest;
            material.bAlphaBlend = mat.bAlphaBlend;
            material.bDoubleSided = mat.bDoubleSided;
            payload.Materials.push_back(eastl::move(material));
        }
        payload.Skeleton.Joints = eastl::move(result.Joints);
        payload.Skeleton.InverseBindMatrices = eastl::move(result.InverseBindMatrices);
        payload.Animations = eastl::move(result.Animations);

        // Header: canonical source path + stat + refs
        FVAssetHeader header;
        header.SourcePath = FAssetPathUtils::MakeAssetsRelative(srcPath, assetsPath);
        struct _stat64 st = {};
        if (_stat64(srcPath.c_str(), &st) == 0)
        {
            header.SourceMTime = static_cast<uint64_t>(st.st_mtime);
            header.SourceSize = static_cast<uint64_t>(st.st_size);
        }
        eastl::hash_set<eastl::string> seenRefs;
        for (const FAssetMaterial& mat : payload.Materials)
        {
            for (const FAssetTextureRef& tex : mat.Textures)
            {
                if (seenRefs.insert(tex.RelativePath).second)
                {
                    header.References.push_back(tex.RelativePath);
                }
            }
        }

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(dstPath.c_str()).parent_path(), ec);

        if (!ValidateModel(payload, "CookModel"))
        {
            if (!bSilent) VTNA_LOG_ERROR("[Cooker] Cooked model failed validation: {}", srcPath);
            return false;
        }

        // Cooked products may live in a nested mirror tree (Assets/CookedAssets/...);
        // create the destination directory on demand before writing.
        const std::filesystem::path dstParent = std::filesystem::path(dstPath.c_str()).parent_path();
        if (!dstParent.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(dstParent, ec);
            if (ec)
            {
                if (!bSilent) VTNA_LOG_ERROR("[Cooker] Failed to create output directory {}: {}", dstParent.string(), ec.message());
                return false;
            }
        }
        if (!FAssetWriter::Write(dstPath, header, payload))
        {
            if (!bSilent) VTNA_LOG_ERROR("[Cooker] Failed to write {}", dstPath);
            return false;
        }
        if (!bSilent && skippedMeshes > 0)
        {
            VTNA_LOG_WARN("[Cooker] {} primitive(s) skipped", skippedMeshes);
        }
        return true;
    }

    bool FVultanaCooker::CookTexture(const eastl::string& srcPath, const eastl::string& dstPath)
    {
        // TODO
        (void)srcPath;
        (void)dstPath;
        VTNA_LOG_WARN("[Cooker] CookTexture lands in Slice 5");
        return false;
    }

    bool FVultanaCooker::IsUpToDate(const eastl::string& srcPath, const eastl::string& dstPath)
    {
        return FAssetWriter::IsUpToDate(srcPath, dstPath);
    }
} // namespace Assets
