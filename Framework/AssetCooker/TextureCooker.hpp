#pragma once

#include "AssetCooker/VAssetTypes.hpp"

#include <EASTL/string.h>

namespace Assets
{
    class FTextureCooker
    {
    public:
        // Decode srcPath (STB) -> full mip chain -> BCn (ispc_texcomp) or float -> FTextureCookedData.
        // bSRGB is implied by usage (BaseColor/Emissive/Diffuse are sRGB; everything else linear).
        // Returns false and logs on any failure (missing file, decode error, oversize payload).
        static bool Cook(const eastl::string& srcPath, ETextureUsage usage, FTextureCookedData& outPayload);
        static bool IsSRGBUsage(ETextureUsage usage);
    };
}