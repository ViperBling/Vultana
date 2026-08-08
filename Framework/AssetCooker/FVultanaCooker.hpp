#pragma once

#include "AssetCooker/FVAssetTypes.hpp"

#include <EASTL/string.h>

namespace Assets
{
    // Cooker facade: shared by the CLI and FAssetManager (cook-on-miss).
    class FVultanaCooker
    {
    public:
        static bool CookModel(const eastl::string& srcPath, const eastl::string& dstPath, bool bSilent = false);
        static bool CookTexture(const eastl::string& srcPath, const eastl::string& dstPath);
        static bool IsUpToDate(const eastl::string& srcPath, const eastl::string& dstPath);
    };
}