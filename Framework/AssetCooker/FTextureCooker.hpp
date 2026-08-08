#pragma once

#include "AssetCooker/FVAssetTypes.hpp"

#include <EASTL/string.h>

namespace Assets
{
    class FTextureCooker
    {
    public:
        static bool Cook(const eastl::string& srcPath, const eastl::string& dstPath);
    };
}