#include "AssetCooker/VultanaCooker.hpp"
#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetCooker/AssetValidate.hpp"
#include "AssetCooker/AssetWriter.hpp"
#include "AssetCooker/Importer.hpp"
#include "AssetCooker/GLTFImporter.hpp"
#include "AssetCooker/AssimpImporter.hpp"
#include "AssetCooker/MeshCooker.hpp"
#include "AssetCooker/TextureCooker.hpp"

#include "Utilities/Log.hpp"

#include <EASTL/vector.h>
#include <EASTL/hash_set.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/memory.h>

#include <cctype>
#include <sys/stat.h>
#include <filesystem>

namespace Assets
{
    bool IsGLTFPath(const eastl::string& srcPath)
    {
        const size_t dot = srcPath.find_last_of('.');
        const size_t slash = srcPath.find_last_of("/\\");
        if (dot == eastl::string::npos || (slash != eastl::string::npos && slash > dot))
        {
            return false; // no extension, or the '.' is inside a directory name
        }
        eastl::string ext = srcPath.substr(dot + 1);
        for (char& c : ext)
        {
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        }
        return ext == "gltf" || ext == "glb";
    }

    eastl::unique_ptr<IImporter> CreateImporter(const eastl::string& srcPath)
    {
        if (IsGLTFPath(srcPath))
        {
            return eastl::make_unique<FGLTFImporter>();
        }
        return eastl::make_unique<FAssimpImporter>();
    }

    bool Assets::FVultanaCooker::CookModel(const eastl::string& srcPath, const eastl::string& dstPath, bool bSilent, const eastl::string& assetsPath)
    {
        const eastl::string& assetsRoot = assetsPath.empty() ? FAssetPathUtils::GetAssetsRootPath() : assetsPath;
        eastl::unique_ptr<IImporter> importer = CreateImporter(srcPath);
        FImportResult result;
        if (!importer->Import(srcPath, result))
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
        header.SourcePath = FAssetPathUtils::MakeAssetsRelative(srcPath, assetsRoot);
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
        // dstPath is the full product path ("<ContentRoot><rel>.<Usage>.vasset"); the usage and
        // the Assets-relative source are parsed from it, so the product path is the source of truth.
        eastl::string srcRel;
        ETextureUsage usage = ETextureUsage::BaseColor;
        if (!FAssetPathUtils::ParseTextureProductPath(dstPath, srcRel, usage))
        {
            VTNA_LOG_ERROR("[Cooker] {} is not a texture product path (expected <rel>.Usage.vasset)", dstPath);
            return false;
        }

        FTextureCookedData payload;
        if (!FTextureCooker::Cook(srcPath, usage, payload))
        {
            VTNA_LOG_ERROR("[Cooker] Failed to cook texture {}", srcPath);
            return false;
        }

        FVAssetHeader header;
        header.Type = EAssetType::Texture;
        header.SourcePath = FAssetPathUtils::MakeAssetsRelative(srcPath, FAssetPathUtils::GetAssetsRootPath());
        struct _stat64 st = {};
        if (_stat64(srcPath.c_str(), &st) == 0)
        {
            header.SourceMTime = static_cast<uint64_t>(st.st_mtime);
            header.SourceSize = static_cast<uint64_t>(st.st_size);
        }
        // Texture products have no dependency references.

        if (!ValidateTexture(payload, "CookTexture"))
        {
            VTNA_LOG_ERROR("[Cooker] Cooked texture failed validation: {}", srcPath);
            return false;
        }

        // Products mirror into a nested Content tree; create the destination directory on demand.
        const std::filesystem::path dstParent = std::filesystem::path(dstPath.c_str()).parent_path();
        if (!dstParent.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(dstParent, ec);
            if (ec)
            {
                VTNA_LOG_ERROR("[Cooker] Failed to create output directory {}: {}", dstParent.string(), ec.message());
                return false;
            }
        }
        if (!FAssetWriter::Write(dstPath, header, payload))
        {
            VTNA_LOG_ERROR("[Cooker] Failed to write {}", dstPath);
            return false;
        }
        return true;
    }

    bool FVultanaCooker::IsUpToDate(const eastl::string& srcPath, const eastl::string& dstPath)
    {
        return FAssetWriter::IsUpToDate(srcPath, dstPath);
    }
} // namespace Assets
