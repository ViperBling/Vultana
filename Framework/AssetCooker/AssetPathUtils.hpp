#pragma once

#include "AssetCooker/VAssetTypes.hpp" // ETextureUsage

#include <EASTL/string.h>

namespace Assets
{
    class FAssetPathUtils
    {
    public:
        static void SetAssetsRootPath(const eastl::string& path);
        static const eastl::string& GetAssetsRootPath();
        static void SetContentRootPath(const eastl::string& path);
        static const eastl::string& GetContentRootPath();
        
        static eastl::string NormalizeAssetPath(const eastl::string& path);
        static eastl::string MakeAssetsRelative(const eastl::string& path, const eastl::string& assetsPath);
        static eastl::string MakeCookedModelPath(const eastl::string& srcPath);
        static eastl::string MakeCookedTexturePath(const eastl::string& srcPath, ETextureUsage usage);
        static eastl::string GetTextureSourcePath(const eastl::string& cookedPath);

        static bool ParseTextureProductPath(const eastl::string& productPath, eastl::string& outSrcRel, ETextureUsage& outUsage);
        static bool IsDDSSource(const eastl::string& srcRel);
        static eastl::string MakeEngineAssetPath(const eastl::string& path);

    private:
        static eastl::string TextureUsageSuffix(ETextureUsage usage);
    };
} // namespace Assets