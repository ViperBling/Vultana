#include "AssetCooker/AssetPathUtils.hpp"

#include <cassert>
#include <cstring>

namespace Assets
{
    eastl::string FAssetPathUtils::NormalizeAssetPath(const eastl::string& path)
    {
        eastl::string result = path;
        for (char& c : result)
            if (c == '\\')
                c = '/';
        while (result.compare(0, 3, "../") == 0)
            result.erase(0, 3);
        return result;
    }

    eastl::string FAssetPathUtils::MakeAssetsRelative(const eastl::string& path, const eastl::string& assetsPath)
    {
        eastl::string normalized = NormalizeAssetPath(path);
        eastl::string assets = NormalizeAssetPath(assetsPath);
        if (normalized.compare(0, assets.size(), assets) == 0)
        {
            normalized.erase(0, assets.size());
            while (!normalized.empty() && normalized[0] == '/')
                normalized.erase(0, 1);
            return normalized;
        }
        // Fallback: strip a literal "../Assets/" prefix (CLI passes CWD-relative paths).
        constexpr const char* kAssetsPrefix = "../Assets/";
        if (normalized.compare(0, strlen(kAssetsPrefix), kAssetsPrefix) == 0)
            return normalized.substr(strlen(kAssetsPrefix));
        return normalized;
    }

    eastl::string FAssetPathUtils::MakeCookedModelPath(const eastl::string& srcPath)
    {
        // Cooked products live in a single mirror tree under Assets/CookedAssets/ (the
        // assets-relative directory is preserved, extension kept: "Sponza.gltf" ->
        // "CookedAssets/.../Sponza.gltf.vasset"; a blind extension erase would collide
        // Box.gltf/Box.glb). Sources outside the assets root — absolute paths, or strings
        // already assets-relative without the "../Assets/" prefix — fall back to the
        // legacy side-by-side product so callers passing either form keep working.
        const eastl::string rel = MakeAssetsRelative(srcPath, "../Assets/");
        if (rel != srcPath)
        {
            return "../Assets/CookedAssets/" + rel + ".vasset";
        }
        return srcPath + ".vasset";
    }

    eastl::string FAssetPathUtils::TextureUsageSuffix(ETextureUsage usage)
    {
        switch (usage)
        {
        case ETextureUsage::BaseColor:              return ".BaseColor";
        case ETextureUsage::MetallicRoughness:      return ".MetallicRoughness";
        case ETextureUsage::Normal:                 return ".Normal";
        case ETextureUsage::Emissive:               return ".Emissive";
        case ETextureUsage::AmbientOcclusion:       return ".AmbientOcclusion";
        case ETextureUsage::Diffuse:                return ".Diffuse";
        case ETextureUsage::SpecularGlossiness:     return ".SpecularGlossiness";
        default:
            assert(false);
            return ".Unknown";
        }
    }

    eastl::string FAssetPathUtils::MakeCookedTexturePath(const eastl::string& srcPath, ETextureUsage usage)
    {
        return srcPath + TextureUsageSuffix(usage) + ".vasset";
    }

    eastl::string FAssetPathUtils::GetTextureSourcePath(const eastl::string& cookedPath)
    {
        // S3-18: match suffixes at the END of the string, not the first occurrence — a source
        // named "albedo.Normal.png" would otherwise have its ".Normal" truncated by find().
        eastl::string result = cookedPath;
        const size_t vasset = result.size() >= 7 ? result.size() - 7 : eastl::string::npos;
        if (vasset != eastl::string::npos && result.compare(vasset, 7, ".vasset") == 0)
            result.erase(vasset);
        static const char* kSuffixes[] = {".BaseColor", ".MetallicRoughness", ".Normal", ".Emissive",
                                          ".AmbientOcclusion", ".Diffuse", ".SpecularGlossiness"};
        for (const char* suffix : kSuffixes)
        {
            const size_t suffixLen = strlen(suffix);
            if (result.size() >= suffixLen && result.compare(result.size() - suffixLen, suffixLen, suffix) == 0)
            {
                result.erase(result.size() - suffixLen);
                break;
            }
        }
        return result;
    }
}