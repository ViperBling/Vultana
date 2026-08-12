#include "AssetCooker/AssetPathUtils.hpp"

#include <EASTL/string.h>
#include <cassert>
#include <cstring>
#include <filesystem>

namespace Assets
{
    eastl::string& GetAssetsRootMutable()
    {
        static eastl::string root = "../Assets/";
        return root;
    }

    eastl::string& GetContentRootMutable()
    {
        static eastl::string root = "../Content/";
        return root;
    }
    
    void FAssetPathUtils::SetAssetsRootPath(const eastl::string& path)
    {
        GetAssetsRootMutable() = path;
    }

    const eastl::string& FAssetPathUtils::GetAssetsRootPath()
    {
        return GetAssetsRootMutable();
    }

    void FAssetPathUtils::SetContentRootPath(const eastl::string& path)
    {
        GetContentRootMutable() = path;
    }

    const eastl::string& FAssetPathUtils::GetContentRootPath()
    {
        return GetContentRootMutable();
    }

    eastl::string FAssetPathUtils::NormalizeAssetPath(const eastl::string& path)
    {
        eastl::string result = path;
        for (char& c : result)
            if (c == '\\')
                c = '/';
        if (result.compare(0, 3, "../") == 0)
            result.erase(0, 3);
        return result;
    }

    eastl::string FAssetPathUtils::MakeAssetsRelative(const eastl::string& path, const eastl::string& assetsPath)
    {
        eastl::string normalized = NormalizeAssetPath(path);
        const eastl::string assets = NormalizeAssetPath(assetsPath);
        if (normalized.compare(0, assets.size(), assets) == 0)
        {
            normalized.erase(0, assets.size());
            while (!normalized.empty() && normalized[0] == '/')
                normalized.erase(0, 1);
            return normalized;
        }
        return normalized;
    }

    eastl::string FAssetPathUtils::MakeCookedModelPath(const eastl::string& srcPath)
    {
        const eastl::string normalized = NormalizeAssetPath(srcPath);
        const eastl::string assetsRoot = NormalizeAssetPath(FAssetPathUtils::GetAssetsRootPath());
        if (normalized.compare(0, assetsRoot.size(), assetsRoot) == 0)
        {
            eastl::string rel = normalized.substr(assetsRoot.size());
            while (!rel.empty() && rel[0] == '/')
                rel.erase(0, 1);
            return FAssetPathUtils::GetContentRootPath() + rel + ".vasset";
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
        const eastl::string rel = MakeAssetsRelative(srcPath, GetAssetsRootPath());
        return rel + TextureUsageSuffix(usage) + ".vasset";
    }

    eastl::string FAssetPathUtils::GetTextureSourcePath(const eastl::string& cookedPath)
    {
        eastl::string srcRel;
        ETextureUsage usage;
        if (ParseTextureProductPath(cookedPath, srcRel, usage))
        {
            return srcRel;
        }
        // Caller passed a path without a usage suffix: strip ".vasset" only (old behavior).
        eastl::string result = cookedPath;
        const size_t vasset = result.size() >= 7 ? result.size() - 7 : eastl::string::npos;
        if (vasset != eastl::string::npos && result.compare(vasset, 7, ".vasset") == 0)
            result.erase(vasset);
        return result;
    }

    bool FAssetPathUtils::ParseTextureProductPath(const eastl::string& productPath, eastl::string& outSrcRel, ETextureUsage& outUsage)
    {
        eastl::string rel = MakeAssetsRelative(productPath, GetContentRootPath());
        const size_t vasset = rel.size() >= 7 ? rel.size() - 7 : eastl::string::npos;
        if (vasset == eastl::string::npos || rel.compare(vasset, 7, ".vasset") != 0)
            return false;
        rel.erase(vasset);

        struct FSuffix
        {
            const char* Text;
            ETextureUsage Usage;
        };
        static const FSuffix kSuffixes[] = 
        {
            {".BaseColor", ETextureUsage::BaseColor},
            {".MetallicRoughness", ETextureUsage::MetallicRoughness},
            {".Normal", ETextureUsage::Normal},
            {".Emissive", ETextureUsage::Emissive},
            {".AmbientOcclusion", ETextureUsage::AmbientOcclusion},
            {".Diffuse", ETextureUsage::Diffuse},
            {".SpecularGlossiness", ETextureUsage::SpecularGlossiness},
        };
        for (const FSuffix& suffix : kSuffixes)
        {
            const size_t suffixLen = strlen(suffix.Text);
            if (rel.size() >= suffixLen && rel.compare(rel.size() - suffixLen, suffixLen, suffix.Text) == 0)
            {
                rel.erase(rel.size() - suffixLen);
                outSrcRel = rel;
                outUsage = suffix.Usage;
                return true;
            }
        }
        return false;
    }

    bool FAssetPathUtils::IsDDSSource(const eastl::string& srcRel)
    {
        const size_t pos = srcRel.find_last_of('.');
        if (pos == eastl::string::npos || srcRel.size() - pos != 4)
            return false;
        return (srcRel[pos + 1] == 'd' || srcRel[pos + 1] == 'D') &&
               (srcRel[pos + 2] == 'd' || srcRel[pos + 2] == 'D') &&
               (srcRel[pos + 3] == 's' || srcRel[pos + 3] == 'S');
    }

    eastl::string FAssetPathUtils::MakeEngineAssetPath(const eastl::string& path)
    {
        auto withTrailingSlash = [](eastl::string s)
        {
            if (!s.empty() && s.back() != '/')
                s.push_back('/');
            return s;
        };
        const eastl::string normalized = NormalizeAssetPath(path);
        const eastl::string assetsRoot = withTrailingSlash(NormalizeAssetPath(FAssetPathUtils::GetAssetsRootPath()));

        if (normalized.compare(0, assetsRoot.size(), assetsRoot) == 0)
        {
            eastl::string rel = normalized.substr(assetsRoot.size());
            while (!rel.empty() && rel[0] == '/')
                rel.erase(0, 1);
            return FAssetPathUtils::GetAssetsRootPath() + rel;
        }
        
        std::error_code ec;
        const eastl::string absAssetsRoot = withTrailingSlash(NormalizeAssetPath(std::filesystem::weakly_canonical(std::filesystem::path(FAssetPathUtils::GetAssetsRootPath().c_str()), ec).string().c_str()));
        const eastl::string absPath = NormalizeAssetPath(std::filesystem::weakly_canonical(std::filesystem::path(normalized.c_str()), ec).string().c_str());
        if (!absAssetsRoot.empty() && !absPath.empty() && absPath.compare(0, absAssetsRoot.size(), absAssetsRoot) == 0)
        {
            eastl::string rel = absPath.substr(absAssetsRoot.size());
            while (!rel.empty() && rel[0] == '/')
                rel.erase(0, 1);
            return FAssetPathUtils::GetAssetsRootPath() + rel; // "../Assets/<rel>"
        }

        std::error_code ecAbs;
        const eastl::string absFallback = NormalizeAssetPath(
            std::filesystem::absolute(std::filesystem::path(normalized.c_str()), ecAbs).string().c_str());
        if (!absFallback.empty())
            return absFallback;
        eastl::string fallback = path;
        for (char& c : fallback)
            if (c == '\\')
                c = '/';
        return fallback;
    }

} // namespace Assets